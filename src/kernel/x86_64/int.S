.extern kwrapper_isr
.code64

kcommon_isr:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsp
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    
    mov %rsp, %rdi
    cld

    call kwrapper_isr

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rsp
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    add $16, %rsp
    iretq

.extern kwrapper_irq

kcommon_irq:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsp
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    
    mov %rsp, %rdi
    cld

    call kwrapper_irq

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rsp
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    add $16, %rsp
    iretq

.irp num,0,1,2,3,4,5,6,7,9,15,16,18,19,20,22,23,24,25,26,27,28,31
.global kisr\num
kisr\num:
    push $0
    push $\num
    jmp kcommon_isr
.endr

.irp num,8,10,11,12,13,14,17,21,29,30
.global kisr\num
kisr\num:
    push $\num
    jmp kcommon_isr
.endr

.irp num,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
.global kirq\num
kirq\num:
    push $0
    push $\num
    jmp kcommon_irq
.endr

#include <mem.h>

.global kdesc_setdescriptors
kdesc_setdescriptors:
    push %rbp
    mov %rsp, %rbp
.extern kgdt
    lgdt phys_from_virt(kgdt)
.extern kidt
    lidt phys_from_virt(kidt)
    pop %rbp
    ret

.global kdesc_reload
kdesc_reload:
    push %rbp
    mov %rsp, %rbp
    movabs $.reset, %rax
    pushq $0x8
    push %rax
    retfq
.reset:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    pop %rbp
    ret