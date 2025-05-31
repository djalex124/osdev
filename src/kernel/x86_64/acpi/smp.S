#include <mem.h>

.code16
.global ap_trampoline
ap_trampoline:
    cli
    cld
    ljmp $0, $0x8040

    .align 16
ap_gdt:
    .long 0, 0
    .quad 0x00209A0000000000
    .quad 0x0000920000000000
    .long 0, 0
ap_gdtp:
    .short ap_gdtp - ap_gdt - 1
    .long 0x8010
    .long 0, 0

    .align 64

    lidt 0x8100

    mov $0xA0, %eax
    mov %eax, %cr4

    mov $0x9000, %edx
    mov %edx, %cr3

    mov $0xC0000080, %ecx
    rdmsr

    or $0x100, %eax
    wrmsr

    mov %cr0, %ebx
    or $0x80000001, %ebx
    mov %ebx, %cr0

    lgdt 0x8030

    ljmp $0x8, $0x8108

.code64
    .align 0x100
count:
    .quad 0
long_mode:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

.extern kidt
    lidt phys_from_virt(kidt)

.extern kgdt
    lgdt phys_from_virt(kgdt)

.extern ptab4
    movabs ptab4, %rax
    mov %rax, %cr3
    
    mov $1, %rax
    cpuid
    shr $24, %rbx

.extern kacpi_apstacks
    shl $12, %rbx

    movabsq kacpi_apstacks, %rax
    addq %rax, %rbx

    movq %rbx, %rsp

.extern bsplock

1:  pause
    movabs bsplock, %rax
    cmp $0, %rax
    jz 1b
    lock incq 0x8100

.extern kacpi_aploop
    movabsq $kacpi_aploop, %rax
    callq *%rax