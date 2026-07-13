#include <efi.h>
#include <efilib.h>

#include <kernel/kernel.h>

typedef uint64_t Elf64_Addr;

#define SELFMAG     4
#define EI_CLASS    4       /* File class byte index */
#define ELFCLASS64  2       /* 64-bit objects */
#define EI_DATA     5       /* Data encoding byte index */
#define ELFDATA2LSB 1       /* 2's complement, little endian */
#define ET_EXEC     2       /* Executable file */
#define PT_LOAD     1       /* Loadable program segment */

typedef struct
{
    uint8_t  e_ident[16];   /* Magic number and other info */
    uint16_t e_type;        /* Object file type */
    uint16_t e_machine;     /* Architecture */
    uint32_t e_version;     /* Object file version */
    uint64_t e_entry;       /* Entry point virtual address */
    uint64_t e_phoff;       /* Program header table file offset */
    uint64_t e_shoff;       /* Section header table file offset */
    uint32_t e_flags;       /* Processor-specific flags */
    uint16_t e_ehsize;      /* ELF header size in bytes */
    uint16_t e_phentsize;   /* Program header table entry size */
    uint16_t e_phnum;       /* Program header table entry count */
    uint16_t e_shentsize;   /* Section header table entry size */
    uint16_t e_shnum;       /* Section header table entry count */
    uint16_t e_shstrndx;    /* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
    uint32_t p_type;        /* Segment type */
    uint32_t p_flags;       /* Segment flags */
    uint64_t p_offset;      /* Segment file offset */
    uint64_t p_vaddr;       /* Segment virtual address */
    uint64_t p_paddr;       /* Segment physical address */
    uint64_t p_filesz;      /* Segment size in file */
    uint64_t p_memsz;       /* Segment size in memory */
    uint64_t p_align;       /* Segment alignment */
} Elf64_Phdr;

#define EFI_BOOT

#include <kernel/kernel.h>
#include <x86_64/acpi/acpi.h>

#define kernel_virtual   0xFFFFFF8000000000 
#define phys_from_virt(x) ((x) - kernel_virtual)
#define virt_from_phys(x) ((x) + kernel_virtual)

#define assert(n) if (EFI_ERROR(n)) { Print(L"\r\n[ERR]: \"%r\" at line %d - aborting...\r\n", n, __LINE__); return n; }

void* boot_memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

INTN boot_guidcmp(EFI_GUID *a, EFI_GUID *b)
{
    INTN value = a->Data1 - b->Data1;
    value += a->Data2 - b->Data2;
    value += a->Data3 - b->Data3;
    for (int i = 0; i < 4; i++)
        value += a->Data4[i] - b->Data4[i];
    return value;
}

EFI_GRAPHICS_OUTPUT_BLT_PIXEL clear = {.Red = 0, .Green = 0, .Blue = 0};
EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
boot_table* table;
UINTN kernel_size = 0;
void (*kentry)(boot_table* table, UINTN* page_table);
#ifdef AQUA_DEBUG
UINTN debug_symbols = 0;
#endif
EFI_INPUT_KEY key;
INTN boot_timer = 4;

EFI_STATUS load_graphics()
{
    EFI_STATUS s;

    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut, EFI_LIGHTCYAN);

    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    s = LibLocateProtocol(&gop_guid, (void**)&gop);
    assert(s);

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *gop_info;
    UINTN gop_info_size, mode_native;
    s = uefi_call_wrapper(gop->QueryMode, 4, gop, 
        gop->Mode==NULL?0:gop->Mode->Mode, &gop_info_size, &gop_info);
    if (s == EFI_NOT_STARTED)
        s = uefi_call_wrapper(gop->SetMode, 2, gop, 0);
    assert(s);
    mode_native = gop->Mode->Mode;

    s = uefi_call_wrapper(gop->SetMode, 2, gop, mode_native);
    assert(s);

    return 0;
}

EFI_STATUS load_kernel(EFI_HANDLE image_handle)
{
    EFI_STATUS s;

    EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_GUID loaded_image_guid = LOADED_IMAGE_PROTOCOL;
    s = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, 
        &loaded_image_guid, (void**)&loaded_image);
    assert(s);

    EFI_FILE_HANDLE root;
    root = LibOpenRoot(loaded_image->DeviceHandle);

    CHAR16 *file_name = L"kernel.bin";
    EFI_FILE_HANDLE file;
    s = uefi_call_wrapper(root->Open, 5, root, &file, 
        file_name, EFI_FILE_MODE_READ, 
        EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    assert(s);

    UINTN header_size = sizeof(Elf64_Ehdr);
    Elf64_Ehdr *elf_header = AllocatePool(header_size);
    s = uefi_call_wrapper(file->Read, 3, file, &header_size, elf_header);
    assert(s);

    Print(L"[OK]: Read kernel.bin\r\n");

    if (elf_header->e_ident[0] != 0x7F ||
        elf_header->e_ident[1] != 'E' ||
        elf_header->e_ident[2] != 'L' ||
        elf_header->e_ident[3] != 'F' ||
        elf_header->e_ident[EI_CLASS] != ELFCLASS64 ||
        elf_header->e_ident[EI_DATA] != ELFDATA2LSB ||
        elf_header->e_type != ET_EXEC ||
        elf_header->e_machine != 62)
        assert(EFI_LOAD_ERROR);

    header_size = elf_header->e_phnum * elf_header->e_phentsize;
    Elf64_Phdr *prog_headers = AllocatePool(header_size);
    s = uefi_call_wrapper(file->SetPosition, 2, file, elf_header->e_phoff);
    assert(s);
    uefi_call_wrapper(file->Read, 3, file, &header_size, prog_headers);
    assert(s);

    UINTN segment = 0x100000;
    for (Elf64_Phdr* prog_header = prog_headers;
         (char*)prog_header < (char*)prog_headers + header_size;
         prog_header = (Elf64_Phdr*)((char*)prog_header + elf_header->e_phentsize))
    {
        if (prog_header->p_type == PT_LOAD)
        {
            UINTN pages = (prog_header->p_memsz + 0x1000 - 1) / 0x1000;
            s = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, 
                EfiLoaderCode, pages, (EFI_PHYSICAL_ADDRESS)phys_from_virt(prog_header->p_vaddr));
            assert(s);
            s = uefi_call_wrapper(file->SetPosition, 2, file, prog_header->p_offset);
            assert(s);
            kernel_size += pages * 0x1000;
            if (prog_header->p_filesz < prog_header->p_memsz)
                ZeroMem((void *)(phys_from_virt(prog_header->p_vaddr) + prog_header->p_filesz), prog_header->p_memsz - prog_header->p_filesz);
            s = uefi_call_wrapper(file->Read, 3, file, 
                &(prog_header->p_filesz), (EFI_PHYSICAL_ADDRESS)phys_from_virt(prog_header->p_vaddr));
            assert(s);
            Print(L"[OK]: Kernel loaded [0x%x - 0x%x]\r\n", 
                (EFI_PHYSICAL_ADDRESS)phys_from_virt(prog_header->p_vaddr), 
                (EFI_PHYSICAL_ADDRESS)phys_from_virt(prog_header->p_vaddr) + prog_header->p_filesz);
        }
    }

    uefi_call_wrapper(file->Close, 1, file);

    kentry = (void*)(elf_header->e_entry - 0xFFFFFF8000000000);

    FreePool(elf_header);
    FreePool(prog_headers);

#ifdef AQUA_DEBUG
    if (debug_symbols)
    {
        CHAR16 *debug_name = L"kernel.map";
        EFI_FILE_HANDLE debug_file;
        s = uefi_call_wrapper(root->Open, 5, root, &debug_file, 
            debug_name, EFI_FILE_MODE_READ, 
            EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
        assert(s);

        Print(L"[OK]: Locate debug symbols\r\n");

        UINTN debug_start = 0x100000 + kernel_size;
        EFI_FILE_INFO *debug_info = LibFileInfo(debug_file);

        UINTN debug_size = (debug_info->FileSize + 0x1000 - 1) / 0x1000;

        if ((UINTN)debug_info == 0)
            assert(EFI_LOAD_ERROR);

        s = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, 
            EfiLoaderCode, debug_size, (EFI_PHYSICAL_ADDRESS)debug_start);
        assert(s);

        s = uefi_call_wrapper(file->Read, 3, debug_file, 
            &(debug_info->FileSize), (EFI_PHYSICAL_ADDRESS)debug_start);
        assert(s);

        Print(L"[OK]: Debug info loaded [0x%x - 0x%x]\r\n", 
            debug_start, debug_start + debug_size * 0x1000);

        kernel_size += debug_size * 0x1000;
    }
#endif

    return 0;
}

EFI_STATUS load_acpi()
{
    EFI_STATUS s;

    Print(L"[OK]: Config table enteries: %d\r\n", ST->NumberOfTableEntries);

    EFI_GUID acpi2 = ACPI_20_TABLE_GUID;
    EFI_GUID acpi1 = ACPI_TABLE_GUID;

    EFI_CONFIGURATION_TABLE configtable;
    
    for (UINTN i = 0; i < ST->NumberOfTableEntries; i++)
    {
        configtable = ST->ConfigurationTable[i];
        if (boot_guidcmp(&acpi2, &configtable.VendorGuid) == 0)
        {
            table->acpi_ver = 2;
            table->rsdp = (EFI_PHYSICAL_ADDRESS)configtable.VendorTable;
        }
        else if (boot_guidcmp(&acpi1, &configtable.VendorGuid) == 0)
        {
            if (table->acpi_ver == 2)
                continue;
            table->acpi_ver = 1;
            table->rsdp = (EFI_PHYSICAL_ADDRESS)configtable.VendorTable;
        }
    }
    
    Print(L"[OK]: ACPI version %d RSDP 0x%x\r\n", table->acpi_ver, table->rsdp);

    if (strncmpa((unsigned char*)table->rsdp, "RSD PTR ", 8) != 0)
        assert(EFI_UNSUPPORTED);

    return 0;
}

EFI_STATUS create_tables_and_exit(EFI_HANDLE image_handle)
{
    EFI_STATUS s;
    
    UINTN start_addr = 0x100000 + kernel_size;
    UINTN *pt4 = (UINTN *)start_addr;
    UINTN *pt3 = (UINTN *)(start_addr + 0x1000);
    UINTN *pt2 = (UINTN *)(start_addr + 0x2000);

    start_addr += 0x3000;

    pt4[0] = (UINTN)pt3 + 0x3;
    pt4[511] = (UINTN)pt3 + 0x3;
    pt3[0] = (UINTN)pt2 + 0x3;
    for (int i = 0; i < 512; i++)
        pt2[i] = 0x83;

    table = (boot_table *)(start_addr);
    start_addr += sizeof(boot_table);

    table->graphics.framebuffer_base = (uint64_t*)gop->Mode->FrameBufferBase;
    table->graphics.horizontal_res = gop->Mode->Info->HorizontalResolution;
    table->graphics.vertical_res = gop->Mode->Info->VerticalResolution;
    table->graphics.ppsl = gop->Mode->Info->PixelsPerScanLine;

    s = load_acpi();
    assert(s);

    UINTN enteries, mapkey, descsize;
    UINT32 descver;
    EFI_MEMORY_DESCRIPTOR *map = LibMemoryMap(&enteries, &mapkey, &descsize, &descver);
    
    table->mmap_enteries = enteries;
    table->mmap_size = descsize;

    UINTN* k_mmap = (UINTN *)start_addr;
    UINTN mmap_pages = ((enteries * descsize) + 0x1000 - 1) / 0x1000;
    uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderCode, mmap_pages, (EFI_PHYSICAL_ADDRESS)k_mmap);
    ZeroMem(k_mmap, enteries * descsize);
    boot_memcpy(k_mmap, map, descsize * enteries);

    table->safe_mem = virt_from_phys(start_addr + mmap_pages * 0x1000);
    table->mmap = (efi_memory_descriptor*)k_mmap;

    if (uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, mapkey) == 2)
    {
        map = LibMemoryMap(&enteries, &mapkey, &descsize, &descver);
        uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, mapkey);
        table->mmap_enteries = enteries;
        table->mmap_size = descsize;
        boot_memcpy(k_mmap, map, descsize * enteries);
        table->mmap = (efi_memory_descriptor*)k_mmap;
    }

    kentry(table, pt4);

    return EFI_UNSUPPORTED;
}

void boot_menu_reset()
{
    uefi_call_wrapper(gop->Blt, 10, gop, &clear, EfiBltVideoFill, 0, 0, 0, 0,
        gop->Mode->Info->HorizontalResolution, gop->Mode->Info->VerticalResolution, 0);
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0, 0);
}

void boot_menu_graphics()
{
    UINTN page = 1;
    UINTN redraw = 1;
    
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *gop_info;
    UINTN gop_info_size;
    
    do
    {
        if (redraw)
        {
            boot_menu_reset();

            Print(L"Available graphics modes:");
            redraw = 0;
        }

        uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0, 2);
        Print(L"Page (%d/%d)\r\n", page, gop->Mode->MaxMode / 10);
        UINTN last = (gop->Mode->MaxMode < page * 10)?gop->Mode->MaxMode:(page * 10);
        for (UINTN index = (page - 1) * 10; index < last; index += 2)
        {
            uefi_call_wrapper(gop->QueryMode, 4, gop, 
                index, &gop_info_size, &gop_info);
            Print(L"[%d]: %4dx%4d", index - (page - 1) * 10, gop_info->HorizontalResolution,
                gop_info->VerticalResolution);
            if (index == gop->Mode->Mode)
                Print(L" * ");
            else
                Print(L" | ");
            if (index + 1 <= last)
            {
                uefi_call_wrapper(gop->QueryMode, 4, gop, 
                    index + 1, &gop_info_size, &gop_info);
                Print(L"[%d]: %4dx%4d", index + 1 - (page - 1) * 10, gop_info->HorizontalResolution,
                    gop_info->VerticalResolution);
                if (index + 1 == gop->Mode->Mode)
                    Print(L" *");
                else
                    Print(L" |");
            }
            
            Print(L"\r\n");
        }

        Print(L"\r\n");
        Print(L"Press [N] to see next page.\r\n");
        Print(L"Press [P] to see previous page.\r\n");
        Print(L"Press [0-9] to select mode.\r\n");
        Print(L"Press [enter] to confirm.\r\n");

        uefi_call_wrapper(BS->WaitForEvent, 3, 1, &ST->ConIn->WaitForKey, NULL);
        uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);

        if (key.UnicodeChar == 0x6E)
        {
            if (gop->Mode->MaxMode >= (page + 1) * 10)
            {
                page++;
                redraw = 1;
            }
        }
        else if (key.UnicodeChar == 0x70)
        {
            if (page - 1 > 0)
            {
                page--;
                redraw = 1;
            }
        }
        else if (key.UnicodeChar >= 0x30 && key.UnicodeChar <= 0x39)
        {
            UINTN selection = key.UnicodeChar - 0x30 + (page - 1) * 10;
            uefi_call_wrapper(gop->SetMode, 2, gop, selection);
            redraw = 1;
        }
    } while (key.UnicodeChar != 0xD);

    key.UnicodeChar = 0;
    
    boot_menu_reset();
    AsciiPrint("<AQUA ");
    AsciiPrint(AQUA_VER_STRING); 
    AsciiPrint(" pre-boot>");
}

void boot_dec_timer()
{
    boot_timer--;
}

void boot_menu()
{
    EFI_EVENT boot_timer_event;
    uefi_call_wrapper(ST->ConIn->Reset, 2, ST->ConIn, FALSE);
    uefi_call_wrapper(ST->ConOut->EnableCursor, 2, ST->ConOut, FALSE);
    uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK,
        (EFI_EVENT_NOTIFY)boot_dec_timer, NULL, &boot_timer_event);

    BOOLEAN key_changed = FALSE;

    boot_menu_reset();
    AsciiPrint("<AQUA ");
    AsciiPrint(AQUA_VER_STRING); 
    AsciiPrint(" pre-boot>");

    do
    {
        uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0, 2);

        Print(L"Current graphics mode: %dx%d\r\n", 
            gop->Mode->Info->HorizontalResolution,
            gop->Mode->Info->VerticalResolution);
#ifdef AQUA_DEBUG
        Print(L"Debugging symbols enabled: %x\r\n", debug_symbols);
#endif

        Print(L"\r\n");
        Print(L"Press [G] to change graphics mode.\r\n");

#ifdef AQUA_DEBUG
        Print(L"Press [1] to toggle debugging symbols.\r\n");
#endif

        Print(L"\r\nPress [enter] to boot.");
        
        if (!key_changed)
        {
            Print(L"\r\n\r\nAutomatically booting in %d seconds.", boot_timer);
            uefi_call_wrapper(BS->SetTimer, 3, boot_timer_event, TimerRelative, 10000000);
        }

        INTN last_timer = boot_timer;
        UINTN last_key = key.UnicodeChar;
        while (last_key == key.UnicodeChar && last_timer == boot_timer)
            uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);

        if (last_key != key.UnicodeChar && !key_changed)
        {
            key_changed = TRUE;

            boot_menu_reset();
            AsciiPrint("<AQUA ");
            AsciiPrint(AQUA_VER_STRING); 
            AsciiPrint(" pre-boot>");
        }

        if (boot_timer == 0)
            break;

        if (key.UnicodeChar == 0x67)
            boot_menu_graphics();
#ifdef AQUA_DEBUG
        else if (key.UnicodeChar == 0x31)
            debug_symbols ^= 1;
#endif
    } while (key.UnicodeChar != 0xD);

    Print(L"\r\n\r\n");
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    ST = system_table;
    BS = ST->BootServices;
    
    EFI_STATUS s;
    
    InitializeLib(image_handle, system_table);
    uefi_call_wrapper(BS->SetWatchdogTimer, 4, 0, 0, 0, NULL);

    s = load_graphics();
    assert(s);

    boot_menu();
    boot_menu_reset();

    s = load_kernel(image_handle);
    assert(s);

    s = create_tables_and_exit(image_handle);
    assert(s);

    assert(EFI_END_OF_FILE);
}