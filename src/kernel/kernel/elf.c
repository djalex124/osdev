#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/elf.h>

#include <output/kterm.h>

#include <mm/mem.h>

extern void *kmem_newpagetable();

void kelf_exec(void *elf_buffer)
{
    Elf64_Ehdr *elf_header = elf_buffer;

    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)elf_buffer + elf_header->e_phoff);
    size_t phdr_size = elf_header->e_phnum * elf_header->e_phentsize;

    //uint64_t *new_ptab4 = kmem_palloc(1, kmem_paging_1kb);
    //kmem_pageentry(virt_from_phys((uint64_t)new_ptab4), (uint64_t)new_ptab4, (uint64_t)new_ptab4, 0x1000, 0b11, kmem_paging_1kb);
    // - should be handled by process.c

    for (Elf64_Phdr* prog_header = phdr;
         (uint64_t)prog_header < (uint64_t)phdr + phdr_size;
         prog_header = (Elf64_Phdr*)((char*)prog_header + elf_header->e_phentsize))
    {
        if (prog_header->p_type == 1)
        {
            //uint64_t *physical = kmem_alloc((prog_header->p_memsz + 0xFFF) / 0x1000);
            //uint64_t physical_addr = (uint64_t)kmem_getphysical(k_ptab4, physical);
            // - get physical from loaded file
            uint64_t size = ((prog_header->p_memsz + 0xFFF) / 0x1000) * 0x1000;
            uint16_t flags = kmem_paging_user;
            if (prog_header->p_flags & 4)
                flags |= kmem_paging_present;
            if (prog_header->p_flags & 2)
                flags |= kmem_paging_writable;
            //r w x nx?
            uint64_t v_addr = (prog_header->p_vaddr / 0x1000) * 0x1000;
            //kmem_pageentry(virt_from_phys((uint64_t)new_ptab4), (uint64_t)physical_addr, v_addr, size, flags, kmem_paging_1kb);
            // - should be handled by process.c
            kterm_putf("\nmap p(alloc) --> v%x sz %x flags %x", v_addr, size, flags);
        }
    }

    //now context switch to new_ptab4
    // - should be handled by process.c

    //kmem_pagesetcr3(k_ptab4);
}

void kelf_run(void *elf_file)
{
    Elf64_Ehdr *elf_header = elf_file;

    if (elf_header->e_ident[0] != 0x7F ||
        elf_header->e_ident[1] != 'E' ||
        elf_header->e_ident[2] != 'L' ||
        elf_header->e_ident[3] != 'F')
    {
        kterm_putf("\nInvalid ELF file.");
        return;
    }
    else if (elf_header->e_ident[EI_CLASS] != ELFCLASS64 ||
        elf_header->e_ident[EI_DATA] != ELFDATA2LSB ||
        elf_header->e_type != ET_EXEC ||
        elf_header->e_machine != 62)
    {
        kterm_putf("\nInvalid x86_64 executable.");
        return;
    } 
    else
        kterm_putf("\nValid ELF file.");

    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)elf_file + elf_header->e_phoff);
    size_t phdr_size = elf_header->e_phnum * elf_header->e_phentsize;

    kterm_putf("\nProgram headers:");

    for (Elf64_Phdr* prog_header = phdr;
         (uint64_t)prog_header < (uint64_t)phdr + phdr_size;
         prog_header = (Elf64_Phdr*)((char*)prog_header + elf_header->e_phentsize))
    {
        kterm_putf("\n[%d] Filesz: 0x%16x Memsz: 0x%16x Align: 0x%x [",
            prog_header->p_type, prog_header->p_filesz, prog_header->p_memsz, prog_header->p_align);
        if (prog_header->p_flags & 4)
            kterm_putf("R");
        else
            kterm_putf(" ");
        if (prog_header->p_flags & 2)
            kterm_putf("W");
        else
            kterm_putf(" ");
        if (prog_header->p_flags & 1)
            kterm_putf("X");
        else
            kterm_putf(" ");
        kterm_putf("]\n    Offset: 0x%16x PAddr: 0x%16x VAddr: 0x%16x", 
            prog_header->p_offset, prog_header->p_paddr, prog_header->p_vaddr);
    }

    kelf_exec(elf_file);
}