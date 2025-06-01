#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <x86_64/acpi/acpi.h>
#include <x86_64/port.h>
#include <x86_64/pit.h>

#include <mm/mem.h>

typedef struct
{
    acpi_madt_header h;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
}acpi_madt_type0;

typedef struct
{
    acpi_madt_header h;
    uint8_t io_apic_id;
    uint8_t zero;
    uint32_t io_apic_addr;
    uint32_t gsi_base;
}acpi_madt_type1;

typedef struct
{
    acpi_madt_header h;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
}acpi_madt_type2;

typedef struct
{
    acpi_madt_header h;
    uint8_t nmi_source;
    uint8_t zero;
    uint16_t flags;
    uint32_t gsi;
}acpi_madt_type3;

typedef struct
{
    acpi_madt_header h;
    uint8_t acpi_processor_id;
    uint16_t flags;
    uint8_t lint;
}__attribute__((packed)) acpi_madt_type4;

typedef struct
{
    acpi_madt_header h;
    uint16_t zero;
    uint64_t local_apic_addr;
}acpi_madt_type5;

typedef struct
{
    acpi_madt_header h;
    uint16_t zero;
    uint64_t processor_x2apic_id;
    uint64_t flags;
    uint64_t acpi_id;
}acpi_madt_type9;

uint32_t *kacpi_apstartup = 0;
uint8_t kacpi_apsrunning = 1;
volatile uint8_t bsplock = 0;

volatile uint64_t kacpi_apstacks = 0;

extern void ap_trampoline();

void kacpi_aploop()
{
    kacpi_apsrunning++;
    while (1)
        asm("hlt");
}

void kacpi_processapic(acpi_madt *madt)
{
    kdebug_outf("\r\nkacpi: processing APIC table\r\n - madt flags %b lapic addr 0x%x", madt->flags, madt->local_apic_addr);
    acpi_madt_header *ptr = (acpi_madt_header *)madt->enteries;
    uint64_t lapic_base = madt->local_apic_addr;
    int total_processors = 0;

    uint8_t *lapic_ids = NULL;

    while ((uint64_t)ptr < (uint64_t)madt + madt->h.length)
    {
        switch (ptr->entry_type)
        {
            case 0:
                acpi_madt_type0 *entry0 = (acpi_madt_type0 *)ptr;
                kdebug_outf("\r\n - processor local apic: processor_id %d id %d flags %2b", 
                    entry0->acpi_processor_id, entry0->apic_id, entry0->flags);
                if ((uint64_t)lapic_ids == 0)
                    lapic_ids = kmem_kalloc(1);
                else
                    kmem_kalloc(1);
                lapic_ids[total_processors] = entry0->apic_id;
                total_processors++;
                break;
            case 5:
                acpi_madt_type5 *entry5 = (acpi_madt_type5 *)ptr;
                kdebug_outf("\r\n - local apic override: addr 0x%x", 
                    entry5->local_apic_addr);
                lapic_base = entry5->local_apic_addr;
                break;
#ifdef AQUA_DEBUG
            case 1:
                acpi_madt_type1 *entry1 = (acpi_madt_type1 *)ptr;
                kdebug_outf("\r\n - i/o apic: apic id %d apic addr 0x%x GSI base 0x%x",
                    entry1->io_apic_id, entry1->io_apic_addr, entry1->gsi_base);
                break;
            case 2:
                acpi_madt_type2 *entry2 = (acpi_madt_type2 *)ptr;
                kdebug_outf("\r\n - i/o apic interrupt source override: bus source %d irq source %d gsi 0x%x flags %b",
                    entry2->bus_source, entry2->irq_source, entry2->gsi, entry2->flags);
                break;
            case 3:
                acpi_madt_type3 *entry3 = (acpi_madt_type3 *)ptr;
                kdebug_outf("\r\n - i/o apic nmi source: nmi source %d flags %b gsi 0x%x", 
                    entry3->nmi_source, entry3->flags, entry3->gsi);
                break;
            case 4:
                acpi_madt_type4 *entry4 = (acpi_madt_type4 *)ptr;
                kdebug_outf("\r\n - local apic nmi: processor_id %d flags %b lint %d", 
                    entry4->acpi_processor_id, entry4->flags, entry4->lint);
                break;
            case 9:
                acpi_madt_type9 *entry9 = (acpi_madt_type9 *)ptr;
                kdebug_outf("\r\n - local x2apic: local x2apic id %d flags %b acpi id 0x%x", 
                    entry9->processor_x2apic_id, entry9->flags, entry9->acpi_id);
                break;
#endif
            default:
                kdebug_outf("\r\n - unknown madt entry (skipping)");
                break;
        }
        ptr = (acpi_madt_header *)((uint64_t)ptr + ptr->entry_length);
    }

    kdebug_outf("\r\nkacpi: attempting to init 0x%x processors", total_processors);

    uint8_t current_apicid = 0;
    asm volatile ("mov $1, %%rax; cpuid; shr $24, %%rbx;" : "=b"(current_apicid) : : );

    if ((uint64_t)kacpi_apstartup == 0)
    {
        kdebug_outf("\r\nkacpi: no memory available for startup code!");
        return;
    }
    kdebug_outf("\r\nkacpi: smp startup code at 0x%x", (uint64_t)kacpi_apstartup);

    //first 2mb should be identity mapped
    memcpy(kacpi_apstartup, &ap_trampoline, 0x1000);
    memset((void *)0x9000, 0, 0x3000);

    uint64_t *pt4 = (uint64_t *)0x9000;
    uint64_t *pt3 = (uint64_t *)0xA000;
    uint64_t *pt2 = (uint64_t *)0xB000;

    pt4[0] = (uint64_t)pt3 + 0x3;
    pt4[511] = (uint64_t)pt3 + 0x3;
    pt3[0] = (uint64_t)pt2 + 0x3;
    pt2[0] = 0x83;

    //1 page of stack per processor to start
    kacpi_apstacks = (uint64_t)kmem_alloc(total_processors);

    kdebug_outf("\r\nkacpi: lapic base at 0x%x", lapic_base);
    kmem_page(lapic_base, lapic_base, 0x8000, 0b11);

    for (int i = 0; i < total_processors; i++)
    {
        if (lapic_ids[i] == current_apicid)
            continue;
        kdebug_outf("\r\nkacpi: starting processor %d id %d", i, lapic_ids[i]);
        *((volatile uint32_t *)(lapic_base + 0x280)) = 0;
        *((volatile uint32_t *)(lapic_base + 0x310)) = 
            (*((volatile uint32_t *)(lapic_base + 0x310)) & 0xFFFFFF) | (i << 24);
        *((volatile uint32_t *)(lapic_base + 0x300)) = 
            (*((volatile uint32_t *)(lapic_base + 0x300)) & 0xFFF00000) | 0xC500;
        do {asm volatile ("pause" : : : "memory");}
        while (*((volatile uint32_t *)(lapic_base + 0x300)) & (1 << 12));
        *((volatile uint32_t *)(lapic_base + 0x310)) = 
            (*((volatile uint32_t *)(lapic_base + 0x310)) & 0xFFFFFF) | (i << 24);
        *((volatile uint32_t *)(lapic_base + 0x300)) = 
            (*((volatile uint32_t *)(lapic_base + 0x300)) & 0xFFF00000) | 0x8500;
        do {asm volatile ("pause" : : : "memory");}
        while (*((volatile uint32_t *)(lapic_base + 0x300)) & (1 << 12));
        ksleep(10);
        for (int j = 0; j < 2; j++)
        {
            *((volatile uint32_t *)(lapic_base + 0x280)) = 0;
            *((volatile uint32_t *)(lapic_base + 0x310)) = 
                (*((volatile uint32_t *)(lapic_base + 0x310)) & 0xFFFFFF) | (i << 24);
            *((volatile uint32_t *)(lapic_base + 0x300)) = 
                (*((volatile uint32_t *)(lapic_base + 0x300)) & 0xFFF0F800) | 0x608;
            ksleep(1);
            do {asm volatile ("pause" : : : "memory");}
            while (*((volatile uint32_t *)(lapic_base + 0x300)) & (1 << 12));
        }
    }

    kmem_kfree(total_processors);

    asm ("sti");

    bsplock = 1;
    //releases all aps that were started to enter kacpi_apicloop
}