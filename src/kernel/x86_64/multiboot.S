#include <mem.h>
#include <multiboot.h>

.section .text
.code32

.align 8
multiboot_header:
    .long multiboot2_header_magic
    .long 0
    .long multiboot_header_end - multiboot_header
    .long -(multiboot2_header_magic + 0 + (multiboot_header_end - multiboot_header))

multiboot_fb:
    .short 5
    .short 1
    .long multiboot_fb_end - multiboot_fb
    .long 1024
    .long 768
    .long 32

multiboot_fb_end:
    .short 0
    .long 8

multiboot_header_end:

.global _start
_start:
    cli
    movl $phys_from_virt(stack_top), %esp

    mov %eax, %edi
    mov %ebx, %esi

    push %edi

    mov $phys_from_virt(boot_tab4), %edi //p4 at 0x1000
    mov %edi, %cr3

    movl $(phys_from_virt(boot_tab3) + 3), (%edi) //p3 at 0x2000
    add $0xFF8, %edi
    movl $(phys_from_virt(boot_tab3) + 3), (%edi) //also map to higher half
    add $0x8, %edi
    movl $(phys_from_virt(boot_tab2) + 3), (%edi) //p2 at 0x3000
    add $0x1000, %edi
    movl $0x83, (%edi)   //large page for first 2 mb
    
    pop %edi

    movl %cr4, %eax
    btsl $5, %eax // PAE mode
    movl %eax, %cr4

    mov $0xC0000080, %ecx // lm bit
    rdmsr
    btsl $8, %eax
    wrmsr

    mov %cr0, %eax
    btsl $31, %eax // paging bit
    mov %eax, %cr0

    lgdt phys_from_virt(gdtp)

    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %ss
    
    ljmp $0x08, $phys_from_virt(_start64)

.section .data
.align 16
gdt:
    .quad 0
    //kernel code pointer
    .word 0
    .word 0
    .word 0x9A00
    .word 0x20
    //kernel data pointer
    .word 0
    .word 0
    .word 0x9200
    .word 0
gdtp:
    .word (gdtp - gdt) - 1
    .long phys_from_virt(gdt)
    .long 0

.section .bss

stack_bottom:
.skip 0x1000
.global stack_top
stack_top:

.align 0x1000
.global boot_tab4
boot_tab4:
.skip 0x1000
boot_tab3:
.skip 0x1000
boot_tab2:
.skip 0x1000