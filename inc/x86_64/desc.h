#pragma once

#include <stdint.h>

typedef struct
{
    uint32_t resv0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t resv1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t resv2;
    uint16_t resv3;
    uint16_t iopb;
}__attribute__((packed)) kernel_tss;

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
}__attribute__((packed)) gdt_entry;

typedef struct
{
    uint16_t limit;
    uint64_t base;
}__attribute__((packed)) gdt_pointer;

typedef struct
{
    uint16_t isr_low;
    uint16_t kernel_CS;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t zero;
}__attribute__((packed)) idt_entry;

typedef struct
{
    uint16_t limit;
    uint64_t base;
}__attribute__((packed)) idt_pointer;

typedef struct
{
    uint64_t r15, r14, r13, r12;
    uint64_t r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rsp;
    uint64_t rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, eflags, user_rsp, ss;
}__attribute__((packed)) kframe_int;

struct kstackframe
{
    struct kstackframe* rbp;
    uint64_t rip;
};

extern void kdesc_setdescriptors();
extern void kdesc_reload();

void kdesc_setinterruptfunc(uint16_t irq, void* function);
void kdesc_removeinterruptfunc(uint16_t irq);
void kdesc_remapinterruptfunc(uint16_t from, uint16_t to);

void kwrapper_seteoi(void *function);

void kdesc_install();