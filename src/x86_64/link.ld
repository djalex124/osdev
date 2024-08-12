OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

kernel_pa = 0x100000;
kernel_va = 0xFFFFFF8000000000;

SECTIONS
{
    . = kernel_pa;

    .multiboot :
    {
        src/x86_64/multiboot.o (.text)
    }

    .debug :
    {
        . = ALIGN(4096);
        _debug_info = .;
        *(.debug_info)
        _debug_abbrev = .;
        *(.debug_abbrev)
    }

    . += kernel_va;

    .text : AT(ADDR(.text) - kernel_va)
    {
        *(EXCLUDE_FILE(*src/x86_64/multiboot.o) .text)
        . = ALIGN(4096);
    }

    .rodata : AT(ADDR(.rodata) - kernel_va)
    {
        _rodata = .;
        *(.rodata*)
        . = ALIGN(4096);
    }

    .data : AT(ADDR(.data) - kernel_va)
    {
        _data = .;
        *(.data)
        . = ALIGN(4096);
    }

    .eh_frame : AT(ADDR(.eh_frame) - kernel_va)
    {
        _ehframe = .;
        *(.eh_frame)
         . = ALIGN(4096);
    }

    .bss : AT(ADDR(.bss) - kernel_va)
    {
        _bss = .;
        *(.bss)

        *(COMMON)
        . = ALIGN(4096);
    }
    
   _end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}