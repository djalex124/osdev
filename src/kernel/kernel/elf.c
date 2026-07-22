#include <kernel/kstring.h>
#include <kernel/elf.h>

#include <output/kterm.h>

void kelf_run(void *elf_file)
{
    Elf64_Ehdr *elf_header = elf_file;

    if (elf_header->e_ident[0] != 0x7F ||
        elf_header->e_ident[1] != 'E' ||
        elf_header->e_ident[2] != 'L' ||
        elf_header->e_ident[3] != 'F')
        kterm_putf("\nInvalid ELF file.");
    else
        kterm_putf("\nValid ELF file.");
}