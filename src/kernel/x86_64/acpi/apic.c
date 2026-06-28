#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <x86_64/acpi/acpi.h>
#include <x86_64/acpi/apic.h>
#include <x86_64/port.h>
#include <x86_64/desc.h>
#include <x86_64/pit.h>

#include <mm/mem.h>

#include <cpuid.h>

uint32_t *kacpi_apstartup = (uint32_t *)0x8000;
uint8_t kacpi_apsrunning = 0;
volatile uint8_t bsplock = 0;

volatile uint64_t kacpi_apstacks = 0;

extern void ap_trampoline();

void kacpi_aploop()
{
    kacpi_apsrunning++;
    
    while (!bsplock)
        asm("nop");

    uint64_t current_apicid = 0;
    asm volatile ("mov $1, %%rax; cpuid; shr $24, %%rbx;" : "=b"(current_apicid) : : );

    //from here the ap should be
    //marked available for tasks
    //(just halted for now)

    while (1)
        asm("hlt");
}

#define kacpi_max_processors 32
uint8_t lapic_ids[kacpi_max_processors];

#define lapic_write(addr, off, val) *((volatile uint32_t *)(addr + off)) = val
#define lapic_read(addr, off) *((volatile uint32_t *)(addr + off))

uint64_t kacpi_lapic_base = 0;
uint64_t kacpi_ioapic_base = 0;
uint8_t kacpi_gsi_base = 0;
uint8_t kacpi_apic_enabled = 0;

void kacpi_startaps(uint8_t total_processors)
{
    kdebug_outf("\nkacpi: attempting to init %d processors", total_processors);
    uint64_t lapic_base = kacpi_lapic_base;

    uint8_t current_apicid = 0;
    asm volatile ("mov $1, %%rax; cpuid; shr $24, %%rbx;" : "=b"(current_apicid) : : );

    kdebug_outf("\nkacpi: smp startup code at 0x%x", (uint64_t)kacpi_apstartup);

    kmem_pageentry(0x8000, 0x8000, 0x4000, kmem_paging_present | kmem_paging_writable, kmem_paging_1kb);

    //first 2mb should be identity mapped
    memcpy(kacpi_apstartup, &ap_trampoline, 0x1000);
    memset((void *)0x9000, 0, 0x3000);

    uint64_t *pt4 = (uint64_t *)0x9000;
    uint64_t *pt3 = (uint64_t *)0xA000;
    uint64_t *pt2 = (uint64_t *)0xB000;

    pt4[0] = (uint64_t)pt3 | kmem_paging_present | kmem_paging_writable;
    pt4[511] = (uint64_t)pt3 | kmem_paging_present | kmem_paging_writable;
    pt3[0] = (uint64_t)pt2 | kmem_paging_present | kmem_paging_writable;
    pt2[0] = (1 << 7) | kmem_paging_present | kmem_paging_writable;

    //1 page of stack per processor to start
    kacpi_apstacks = (uint64_t)kmem_alloc(total_processors - 1);

    kdebug_outf("\nkacpi: lapic base at 0x%x", lapic_base);
    kmem_pageentry(lapic_base, lapic_base, 0x1000,
        kmem_paging_present | kmem_paging_writable | kmem_paging_no_cache, kmem_paging_1kb);

    for (int i = 0; i < total_processors; i++)
    {
        if (lapic_ids[i] == current_apicid)
        {
            kdebug_outf("\nkacpi: processor %d id %d already started", i, lapic_ids[i]);
            continue;
        }
        kdebug_outf("\nkacpi: starting processor %d id %d", i, lapic_ids[i]);
        lapic_write(lapic_base, APIC_REG_ERROR, 0);
        lapic_write(lapic_base, APIC_REG_ICR_2, (lapic_read(lapic_base, APIC_REG_ICR_2) & 0xFFFFFF) | (i << 24));
        lapic_write(lapic_base, APIC_REG_ICR_1, (lapic_read(lapic_base, APIC_REG_ICR_1) & 0xFFF00000) | 0xC500);
        do {asm volatile ("pause" : : : "memory");}
        while (lapic_read(lapic_base, APIC_REG_ICR_1) & (1 << 12));
        lapic_write(lapic_base, APIC_REG_ICR_2, (lapic_read(lapic_base, APIC_REG_ICR_2) & 0xFFFFFF) | (i << 24));
        lapic_write(lapic_base, APIC_REG_ICR_1, (lapic_read(lapic_base, APIC_REG_ICR_1) & 0xFFF00000) | 0x8500);
        do {asm volatile ("pause" : : : "memory");}
        while (lapic_read(lapic_base, APIC_REG_ICR_1) & (1 << 12));
        ksleep(10);
        for (int j = 0; j < 2; j++)
        {
            lapic_write(lapic_base, APIC_REG_ERROR, 0);
            lapic_write(lapic_base, APIC_REG_ICR_2, (lapic_read(lapic_base, APIC_REG_ICR_2) & 0xFFFFFF) | (i << 24));
            lapic_write(lapic_base, APIC_REG_ICR_1, (lapic_read(lapic_base, APIC_REG_ICR_1) & 0xFFF0F800) | 0x608);
            ksleep(1);
            do {asm volatile ("pause" : : : "memory");}
            while (lapic_read(lapic_base, APIC_REG_ICR_1) & (1 << 12));
        }
    }
}

void kacpi_apic_eoi()
{
    lapic_write(kacpi_lapic_base, APIC_REG_EOI, 0);
}

void kacpi_ioapic_write(uint32_t off, uint32_t val)
{
    volatile uint32_t *ioapic = (volatile uint32_t *)kacpi_ioapic_base;
    ioapic[0] = (off & 0xff);
    ioapic[4] = val;
}

uint32_t kacpi_ioapic_read(uint32_t off)
{
    volatile uint32_t *ioapic = (volatile uint32_t *)kacpi_ioapic_base;
    ioapic[0] = (off & 0xff);
    return ioapic[4];
}

void kacpi_ioredtbl_entry(uint8_t entry, union ioredtbl_entry *tbl_entry)
{
    kacpi_ioapic_write(0x10 + (entry * 2), tbl_entry->lower);
    kacpi_ioapic_write(0x10 + (entry * 2) + 1, tbl_entry->upper);
}

void kacpi_ioredtbl(acpi_madt *madt)
{
    kmem_pageentry(kacpi_ioapic_base, kacpi_ioapic_base, 0x1000,
        kmem_paging_present | kmem_paging_writable | kmem_paging_no_cache, kmem_paging_1kb);

    kacpi_apic_enabled = 1;

    if (madt->flags)
    {
        for (int i = 0; i < 16; i++)
        {
            union ioredtbl_entry isa_entry;
            isa_entry.int_vector = 0x20 + i;
            isa_entry.deliv_mode = 0;
            isa_entry.desti_mode = 0;
            isa_entry.deliv_stat = 0;
            isa_entry.pin_polarity = 0;
            isa_entry.remote_irr = 0;
            isa_entry.trigger_mode = 0;
            isa_entry.mask = 0;
            isa_entry.reserved = 0;
            isa_entry.destination = 0;

            kacpi_ioredtbl_entry(i, &isa_entry);
        }
    }

    acpi_madt_header *ptr = (acpi_madt_header *)madt->enteries;
    while ((uint64_t)ptr < (uint64_t)madt + madt->h.length)
    {
        switch (ptr->entry_type)
        {
            case 2:
                acpi_madt_type2 *entry2 = (acpi_madt_type2 *)ptr;

                uint8_t flag_pin = (entry2->flags) & 0b11;
                uint8_t flag_trigger = ((entry2->flags) & 0b1100) >> 2;

                union ioredtbl_entry isa_entry;
                isa_entry.int_vector = 0x20 + entry2->gsi;
                isa_entry.deliv_mode = 0;
                isa_entry.desti_mode = 0;
                isa_entry.deliv_stat = 0;
                isa_entry.pin_polarity = (flag_pin == 0 || flag_pin == 1) ? 0 : 1;
                isa_entry.remote_irr = 0;
                isa_entry.trigger_mode = (flag_trigger == 0 || flag_trigger == 1) ? 0 : 1;
                isa_entry.mask = 0;
                isa_entry.reserved = 0;
                isa_entry.destination = 0;

                kacpi_ioredtbl_entry(entry2->irq_source, &isa_entry);

                if (entry2->irq_source != entry2->gsi)
                    kdesc_remapinterruptfunc(entry2->irq_source, entry2->gsi);
                
                break;
            default:
                break;
        }
        ptr = (acpi_madt_header *)((uint64_t)ptr + ptr->entry_length);
    }
}

void kacpi_processapic(acpi_madt *madt)
{
    kdebug_outf("\nkacpi: processing APIC table\n - madt flags %b lapic addr 0x%x", madt->flags, madt->local_apic_addr);
    acpi_madt_header *ptr = (acpi_madt_header *)madt->enteries;
    uint64_t lapic_base = madt->local_apic_addr;
    int total_processors = 0;

    while ((uint64_t)ptr < (uint64_t)madt + madt->h.length)
    {
        switch (ptr->entry_type)
        {
            case 0:
                acpi_madt_type0 *entry0 = (acpi_madt_type0 *)ptr;
                kdebug_outf("\n - processor local apic: processor_id %d id %d flags %2b", 
                    entry0->acpi_processor_id, entry0->apic_id, entry0->flags);
                if (total_processors == kacpi_max_processors)
                {
                    kdebug_outf(" (not started) - reached processor limit");
                    break;
                }
                lapic_ids[total_processors] = entry0->apic_id;
                total_processors++;
                break;
            case 5:
                acpi_madt_type5 *entry5 = (acpi_madt_type5 *)ptr;
                kdebug_outf("\n - local apic override: addr 0x%x", 
                    entry5->local_apic_addr);
                lapic_base = entry5->local_apic_addr;
                break;
            case 1:
                acpi_madt_type1 *entry1 = (acpi_madt_type1 *)ptr;
                kdebug_outf("\n - i/o apic: apic id %d apic addr 0x%x GSI base 0x%x",
                    entry1->io_apic_id, entry1->io_apic_addr, entry1->gsi_base);
                kacpi_ioapic_base = entry1->io_apic_addr;
                kacpi_gsi_base = entry1->gsi_base;
                break;
#ifdef AQUA_DEBUG
            case 2:
                acpi_madt_type2 *entry2 = (acpi_madt_type2 *)ptr;
                kdebug_outf("\n - i/o apic interrupt source override: bus source %d irq source %d gsi 0x%x flags %b",
                    entry2->bus_source, entry2->irq_source, entry2->gsi, entry2->flags);
                break;
            case 3:
                acpi_madt_type3 *entry3 = (acpi_madt_type3 *)ptr;
                kdebug_outf("\n - i/o apic nmi source: nmi source %d flags %b gsi 0x%x", 
                    entry3->nmi_source, entry3->flags, entry3->gsi);
                break;
            case 4:
                acpi_madt_type4 *entry4 = (acpi_madt_type4 *)ptr;
                kdebug_outf("\n - local apic nmi: processor_id %d flags %b lint %d", 
                    entry4->acpi_processor_id, entry4->flags, entry4->lint);
                break;
            case 9:
                acpi_madt_type9 *entry9 = (acpi_madt_type9 *)ptr;
                kdebug_outf("\n - local x2apic: local x2apic id %d flags %b acpi id 0x%x", 
                    entry9->processor_x2apic_id, entry9->flags, entry9->acpi_id);
                break;
#endif
            default:
                kdebug_outf("\n - unknown madt entry (skipping)");
                break;
        }
        ptr = (acpi_madt_header *)((uint64_t)ptr + ptr->entry_length);
    }

    kacpi_lapic_base = lapic_base;
    kacpi_startaps(total_processors);

    kdebug_outf("\nkacpi: configuring apic");

    asm("cli");

    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    lapic_write(kacpi_lapic_base, APIC_REG_SIV, lapic_read(kacpi_lapic_base, APIC_REG_SIV) | 0x1FF);
    lapic_write(kacpi_lapic_base, APIC_REG_EOI, 0);

    kacpi_ioredtbl(madt);

    asm("sti");

    bsplock = 1;
    //releases all aps that were started to enter kacpi_apicloop
}