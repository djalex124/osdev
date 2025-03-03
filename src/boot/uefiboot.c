#include <efi.h>
#include <efilib.h>

#include <elf.h>

#define EFI_BOOT

#include <kernel.h>

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

void* boot_memset(void* bufptr, uint32_t value, size_t size) {
	uint32_t* buf = (uint32_t*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (uint32_t) value;
	return bufptr;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    ST = system_table;
    BS = ST->BootServices;

    EFI_STATUS s;
    
    InitializeLib(image_handle, system_table);
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut, EFI_LIGHTCYAN);

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    s = LibLocateProtocol(&gop_guid, (void**)&gop);
    assert(s);

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *gop_info;
    UINTN gop_info_size, mode_num, mode_native;
    s = uefi_call_wrapper(gop->QueryMode, 4, gop, 
        gop->Mode==NULL?0:gop->Mode->Mode, &gop_info_size, &gop_info);
    if (s == EFI_NOT_STARTED)
        s = uefi_call_wrapper(gop->SetMode, 2, gop, 0);
    assert(s);
    mode_native = gop->Mode->Mode;
    mode_num = gop->Mode->MaxMode;

    s = uefi_call_wrapper(gop->SetMode, 2, gop, mode_native);
    assert(s);

    Print(L"Welcome to concatenOS loader!\r\n");
    Print(L"[OK]: GOP - address 0x%x size 0x%x width %dx%d ppsl %d format %x\r\n",
        gop->Mode->FrameBufferBase, gop->Mode->FrameBufferSize, gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution, gop->Mode->Info->PixelsPerScanLine, gop->Mode->Info->PixelFormat);

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

    if (elf_header->e_ident[EI_MAG0] != ELFMAG0 ||
        elf_header->e_ident[EI_MAG1] != ELFMAG1 ||
        elf_header->e_ident[EI_MAG2] != ELFMAG2 ||
        elf_header->e_ident[EI_MAG3] != ELFMAG3 ||
        elf_header->e_ident[EI_CLASS] != ELFCLASS64 ||
        elf_header->e_ident[EI_DATA] != ELFDATA2LSB ||
        elf_header->e_type != ET_EXEC ||
        elf_header->e_machine != EM_X86_64 ||
        elf_header->e_version != EV_CURRENT)
        assert(EFI_LOAD_ERROR);

    header_size = elf_header->e_phnum * elf_header->e_phentsize;
    Elf64_Phdr *prog_headers = AllocatePool(header_size);
    s = uefi_call_wrapper(file->SetPosition, 2, file, elf_header->e_phoff);
    assert(s);
    uefi_call_wrapper(file->Read, 3, file, &header_size, prog_headers);
    assert(s);

    UINTN segment = 0x100000;
    UINTN kernel_size = 0;
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
            {
                boot_memset((void *)(phys_from_virt(prog_header->p_vaddr) + prog_header->p_filesz), 0, prog_header->p_memsz - prog_header->p_filesz);
                Print(L"[OK]: Clearing memory 0x%x length %x\r\n", 
                    phys_from_virt(prog_header->p_vaddr) + prog_header->p_filesz, prog_header->p_memsz - prog_header->p_filesz);
            }
            s = uefi_call_wrapper(file->Read, 3, file, 
                &(prog_header->p_filesz), (EFI_PHYSICAL_ADDRESS)phys_from_virt(prog_header->p_vaddr));
            assert(s);
            Print(L"[OK]: Kernel loaded (total 0x%x) at 0x%x\r\n", prog_header->p_memsz, (EFI_PHYSICAL_ADDRESS)phys_from_virt(prog_header->p_vaddr));
        }
    }

    uefi_call_wrapper(file->Close, 1, file);

    void (*kentry)(kernel_table* table, UINTN* page_table) = (void*)(elf_header->e_entry - 0xFFFFFF8000000000);
    Print(L"[OK]: Kernel entry at 0x%x\r\n", elf_header->e_entry);

    FreePool(elf_header);
    FreePool(prog_headers);

    UINTN start_addr = segment + kernel_size;
    Print(L"[OK]: Making page tables at 0x%x\r\n", start_addr);

    uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderCode, 4, (EFI_PHYSICAL_ADDRESS)start_addr);

    UINTN *pt4 = (UINTN *)start_addr;
    UINTN *pt3 = (UINTN *)(start_addr + 0x1000);
    UINTN *pt2 = (UINTN *)(start_addr + 0x2000);

    start_addr += 0x3000;

    pt4[0] = (UINTN)pt3 + 0x3;
    pt4[511] = (UINTN)pt3 + 0x3;
    pt3[0] = (UINTN)pt2 + 0x3;
    pt2[0] = 0x83; // page of first 2mb?

    Print(L"[OK]: Kernel structures at 0x%x\r\n", start_addr);

    kernel_table* table = (kernel_table *)(start_addr);
    start_addr += sizeof(kernel_table);

    table->graphics.framebuffer_base = (uint64_t*)gop->Mode->FrameBufferBase;
    table->graphics.horizontal_res = gop->Mode->Info->HorizontalResolution;
    table->graphics.vertical_res = gop->Mode->Info->VerticalResolution;
    table->graphics.ppsl = gop->Mode->Info->PixelsPerScanLine;
    
    UINTN enteries, mapkey, descsize;
    UINT32 descver;
    EFI_MEMORY_DESCRIPTOR *map = LibMemoryMap(&enteries, &mapkey, &descsize, &descver);
    
    table->mmap_enteries = enteries;
    table->mmap_size = descsize;

    UINTN* k_mmap = (UINTN *)start_addr;
    UINTN mmap_pages = ((enteries * descsize) + 0x1000 - 1) / 0x1000;
    uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderCode, mmap_pages, (EFI_PHYSICAL_ADDRESS)k_mmap);
    boot_memset(k_mmap, 0, mmap_pages * 0x1000);
    boot_memcpy(k_mmap, map, descsize * enteries);

    Print(L"[OK]: Kernel mem at 0x%x %x size", (EFI_PHYSICAL_ADDRESS)k_mmap, descsize * enteries);
    table->safe_mem = virt_from_phys(start_addr + mmap_pages * 0x1000);

    table->mmap = (memory_descriptor*)k_mmap;

    if (uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, mapkey) == 2)
    {
        map = LibMemoryMap(&enteries, &mapkey, &descsize, &descver);
        uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, mapkey);
        table->mmap_enteries = enteries;
        table->mmap_size = descsize;
        boot_memcpy(k_mmap, map, descsize * enteries);
        table->mmap = (memory_descriptor*)k_mmap;
    }

    kentry(table, pt4);

    return EFI_UNSUPPORTED;
}