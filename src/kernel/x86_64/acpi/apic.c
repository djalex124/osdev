#include <debug.h>
#include <acpi.h>

void kacpi_processapic(acpi_madt *madt)
{
    kdebug_outf("\r\nkacpi: processing APIC table\r\n - madt %x %x", madt->flags, (uint64_t)madt + madt->h.length);
    acpi_madt_header *ptr = (acpi_madt_header *)madt->enteries;
    while ((uint64_t)ptr < (uint64_t)madt + madt->h.length)
    {
        if (ptr->entry_type == 0)
        {
            acpi_madt_type0 *entry = (acpi_madt_type0 *)ptr;
            kdebug_outf("\r\n - processor local apic %d %d flags %2b", 
                entry->acpi_processor_id, entry->apic_id, entry->flags);
        }
        ptr = (acpi_madt_header *)((uint64_t)ptr + ptr->entry_length);
    }
}