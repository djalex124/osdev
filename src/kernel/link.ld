OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start64)

kernel_va = 0xFFFFFF8000000000;

SECTIONS
{
    . = 0xFFFFFF8000100000;

    .text : AT(ADDR(.text))
    {
        *(.text)
        . = ALIGN(4096);
    }

    .rodata : AT(ADDR(.rodata))
    {
        _rodata = .;
        *(.rodata*)
        . = ALIGN(4096);
    }

    .data : AT(ADDR(.data))
    {
        _data = .;
        *(.data)
        . = ALIGN(4096);
    }

    .eh_frame : AT(ADDR(.eh_frame))
    {
        _ehframe = .;
        *(.eh_frame)
         . = ALIGN(4096);
    }

    .bss : AT(ADDR(.bss))
    {
        _bss = .;
        *(.bss)
        *(COMMON)
        . = ALIGN(4096);
    }

    .font : AT(ADDR(.font))
    {
        bin/font.o
        . = ALIGN(4096);
    }
    
   _end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}