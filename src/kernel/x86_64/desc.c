#include <stdint.h>

#include <output/screen.h>

#include <kernel/debug.h>

#include <x86_64/desc.h>
#include <x86_64/port.h>

const char* kdesc_ints[] =
{
    "DE", "DB", "NM", "BP",
    "OF", "BR", "UD", "NM",
    "DF", "XX", "TS", "NP",
    "SS", "GP", "PF", "XX",
    "MF", "AC", "MC", "XF",
    "VE", "CP", "XX", "XX",
    "XX", "XX", "XX", "XX",
    "HV", "VC", "SX", "XX"
};

void kwrapper_isr(kframe_int *k)
{   
    kscreen_putf("\r\n%n%m --- exception --- ", 0xFF0000, 0x0);
    kscreen_putf("\r\nkisr: isr 0x%d #%s code 0b%b", k->int_no, kdesc_ints[k->int_no], k->err_code);
#ifdef AQUA_DEBUG
    kdebug_outf("\r\nkisr: rax 0x%16x rbx 0x%16x rcx 0x%16x rdx 0x%16x",
        k->rax, k->rbx, k->rcx, k->rdx);
    kdebug_outf("\r\nkisr: rsp 0x%16x rbp 0x%16x rsi 0x%16x rdi 0x%16x",
        k->rsp, k->rbp, k->rsi, k->rdi);
    kdebug_outf("\r\nkisr: r8  0x%16x r9  0x%16x r10 0x%16x r11 0x%16x",
        k->r8, k->r9, k->r10, k->r11);
    kdebug_outf("\r\nkisr: r12 0x%16x r13 0x%16x r14 0x%16x r15 0x%16x",
        k->r12, k->r13, k->r14, k->r15);
    kdebug_outf("\r\nkisr: rip 0x%16x cs  0x%x ss 0x%x",
        k->rip, k->cs, k->ss);
    kdebug_outf("\r\nkisr: eflags 0b%b user_rsp 0x%x",
        k->eflags, k->user_rsp);
    if (k->int_no == 0xD && k->err_code != 0)
    {
        kdebug_outf("\r\nkisr: gpf from ");
        uint16_t which = (k->err_code >> 1) & 3;
        if (which == 0b00)
            kdebug_outf("gdt");
        else if (which == 0b01 || which == 0b11)
            kdebug_outf("idt");
        else if (which == 0b10)
            kdebug_outf("ldt");
        kdebug_outf(" at index %d", k->err_code >> 3);
    }
    else if (k->int_no == 0xE)
    {
        uint64_t cr2;
        asm volatile ("mov %%cr2, %0" : "=r"(cr2));
        kdebug_outf("\r\nkisr: cr2 [0x%x]", cr2);
        kdebug_outf("\r\nkisr: pf code: |");
        if (k->err_code & 1)
        {    
            kdebug_outf("present|");
            if (k->err_code & (1 << 2))
                kdebug_outf("user|");
            else
                kdebug_outf("system|");
        }
        else
            kdebug_outf("non-present|");
        if (k->err_code & (1 << 1))
            kdebug_outf("write|");
        else
            kdebug_outf("read|");
        if (k->err_code & (1 << 3))
            kdebug_outf("reserved bits|");
        if (k->err_code & (1 << 4))
            kdebug_outf("instruction fetch|");
        if (k->err_code & (1 << 5))
            kdebug_outf("pk violation|");
        if (k->err_code & (1 << 6))
            kdebug_outf("ss access|");
        if (k->err_code & (1 << 14))
            kdebug_outf("sgx violation|");
    }
#endif

    //should attempt fix or ret if non crashing isr before stack trace and hlt

    struct kstackframe* stack = (struct kstackframe*)k->rbp;
    kscreen_putf("\r\nkisr: stack trace");
    kscreen_putf("\r\nkisr: [0x%x]", k->rip);
    for(unsigned frame = 0; stack && frame < 5; ++frame)
    {
        kscreen_putf("\r\nkisr: [0x%x]", stack->rip);
        stack = stack->rbp;
    }

    while (1) asm("hlt");
}

typedef void (*kdesc_irqfunc)(void);
kdesc_irqfunc kdesc_irqs[16];

void kwrapper_irq(kframe_int *k)
{
    if (kdesc_irqs[k->int_no])
    {
        void (*function)() = kdesc_irqs[k->int_no];
        function();
    }
    else
        kdebug_outf("\r\nkdesc: irq %d fired but no handler", k->int_no);

    if (k->int_no >= 8)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

gdt_entry kgdt_table[6];
gdt_pointer kgdt;

void kdesc_setgdt(int entry, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran)
{
    kgdt_table[entry].base_low = base & 0xFFFF;
    kgdt_table[entry].base_mid = (base >> 16) & 0xFF;
    kgdt_table[entry].base_high = (base >> 24) & 0xFF;

    kgdt_table[entry].limit_low = limit & 0xFFFF;
    kgdt_table[entry].gran = (limit >> 16) & 0x0F;
    kgdt_table[entry].gran |= gran & 0xF0;
    kgdt_table[entry].access = access;
}

idt_entry __attribute__((aligned(0x10))) kidt_table[256];
idt_pointer kidt;

void kdesc_setidt(int entry, uint64_t handler, uint8_t flags)
{
    kidt_table[entry].isr_low = handler & 0xFFFF;
    kidt_table[entry].isr_mid = (handler >> 16) & 0xFFFF;
    kidt_table[entry].isr_high = (handler >> 32) & 0xFFFFFFFF;

    kidt_table[entry].attributes = flags;
    kidt_table[entry].kernel_CS = 0x8;
    kidt_table[entry].zero = 0;
}

static kernel_tss k_temptss;

extern void kisr0();
extern void kisr1();
extern void kisr2();
extern void kisr3();
extern void kisr4();
extern void kisr5();
extern void kisr6();
extern void kisr7();
extern void kisr8();
extern void kisr9();
extern void kisr10();
extern void kisr11();
extern void kisr12();
extern void kisr13();
extern void kisr14();
extern void kisr15();
extern void kisr16();
extern void kisr17();
extern void kisr18();
extern void kisr19();
extern void kisr20();
extern void kisr21();
extern void kisr22();
extern void kisr23();
extern void kisr24();
extern void kisr25();
extern void kisr26();
extern void kisr27();
extern void kisr28();
extern void kisr29();
extern void kisr30();
extern void kisr31();

extern void kirq0();
extern void kirq1();
extern void kirq2();
extern void kirq3();
extern void kirq4();
extern void kirq5();
extern void kirq6();
extern void kirq7();
extern void kirq8();
extern void kirq9();
extern void kirq10();
extern void kirq11();
extern void kirq12();
extern void kirq13();
extern void kirq14();
extern void kirq15();

void kdesc_setinterruptfunc(uint16_t irq, void* function)
{
    if (irq >= 0 && irq <= 15)
        kdesc_irqs[irq] = (kdesc_irqfunc)function;
}

void kdesc_removeinterruptfunc(uint16_t irq)
{
    if (irq >= 0 && irq <= 15)
        kdesc_ints[irq] = 0;
}

void kdesc_install()
{
    kgdt.base = (uint64_t)&kgdt_table;
    kgdt.limit = (sizeof(gdt_entry) * 6) - 1;

    kdesc_setgdt(0, 0, 0, 0, 0);
    kdesc_setgdt(1, 0, 0xFFFFFFFF, 0x9A, 0xAF);
    kdesc_setgdt(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    kdesc_setgdt(3, 0, 0xFFFFFFFF, 0xFA, 0xAF);
    kdesc_setgdt(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    kdesc_setgdt(5, (uint64_t)&k_temptss, sizeof(kernel_tss), 0x89, 0);

    kidt.base = (uint64_t)&kidt_table;
    kidt.limit = (sizeof(idt_entry) * 256) - 1;

    kdesc_setidt(0, (uint64_t)kisr0, 0x8E);
    kdesc_setidt(1, (uint64_t)kisr1, 0x8E);
    kdesc_setidt(2, (uint64_t)kisr2, 0x8E);
    kdesc_setidt(3, (uint64_t)kisr3, 0x8E);
    kdesc_setidt(4, (uint64_t)kisr4, 0x8E);
    kdesc_setidt(5, (uint64_t)kisr5, 0x8E);
    kdesc_setidt(6, (uint64_t)kisr6, 0x8E);
    kdesc_setidt(7, (uint64_t)kisr7, 0x8E);
    kdesc_setidt(8, (uint64_t)kisr8, 0x8E);
    kdesc_setidt(9, (uint64_t)kisr9, 0x8E);
    kdesc_setidt(10, (uint64_t)kisr10, 0x8E);
    kdesc_setidt(11, (uint64_t)kisr11, 0x8E);
    kdesc_setidt(12, (uint64_t)kisr12, 0x8E);
    kdesc_setidt(13, (uint64_t)kisr13, 0x8E);
    kdesc_setidt(14, (uint64_t)kisr14, 0x8E);
    kdesc_setidt(15, (uint64_t)kisr15, 0x8E);
    kdesc_setidt(16, (uint64_t)kisr16, 0x8E);
    kdesc_setidt(17, (uint64_t)kisr17, 0x8E);
    kdesc_setidt(18, (uint64_t)kisr18, 0x8E);
    kdesc_setidt(19, (uint64_t)kisr19, 0x8E);
    kdesc_setidt(20, (uint64_t)kisr20, 0x8E);
    kdesc_setidt(21, (uint64_t)kisr21, 0x8E);
    kdesc_setidt(22, (uint64_t)kisr22, 0x8E);
    kdesc_setidt(23, (uint64_t)kisr23, 0x8E);
    kdesc_setidt(24, (uint64_t)kisr24, 0x8E);
    kdesc_setidt(25, (uint64_t)kisr25, 0x8E);
    kdesc_setidt(26, (uint64_t)kisr26, 0x8E);
    kdesc_setidt(27, (uint64_t)kisr27, 0x8E);
    kdesc_setidt(28, (uint64_t)kisr28, 0x8E);
    kdesc_setidt(29, (uint64_t)kisr29, 0x8E);
    kdesc_setidt(30, (uint64_t)kisr30, 0x8E);
    kdesc_setidt(31, (uint64_t)kisr31, 0x8E);
    
    kdesc_setidt(32, (uint64_t)kirq0, 0x8E);
    kdesc_setidt(33, (uint64_t)kirq1, 0x8E);
    kdesc_setidt(34, (uint64_t)kirq2, 0x8E);
    kdesc_setidt(35, (uint64_t)kirq3, 0x8E);
    kdesc_setidt(36, (uint64_t)kirq4, 0x8E);
    kdesc_setidt(37, (uint64_t)kirq5, 0x8E);
    kdesc_setidt(38, (uint64_t)kirq6, 0x8E);
    kdesc_setidt(39, (uint64_t)kirq7, 0x8E);
    kdesc_setidt(40, (uint64_t)kirq8, 0x8E);
    kdesc_setidt(41, (uint64_t)kirq9, 0x8E);
    kdesc_setidt(42, (uint64_t)kirq10, 0x8E);
    kdesc_setidt(43, (uint64_t)kirq11, 0x8E);
    kdesc_setidt(44, (uint64_t)kirq12, 0x8E);
    kdesc_setidt(45, (uint64_t)kirq13, 0x8E);
    kdesc_setidt(46, (uint64_t)kirq14, 0x8E);
    kdesc_setidt(47, (uint64_t)kirq15, 0x8E);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x4);
    outb(0xA1, 0x2);
    outb(0x21, 0x1);
    outb(0xA1, 0x1);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
    
    kdesc_setdescriptors();
    kdesc_reload();

    for (int i = 0; i < 16; i++)
        kdesc_setinterruptfunc(i, (void *)0);

    kdebug_outf("\r\nkdesc: interrupt descriptors set");
}