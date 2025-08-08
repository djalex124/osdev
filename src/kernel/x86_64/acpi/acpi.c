#include <x86_64/acpi/acpi.h>

#include <x86_64/port.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <mm/mem.h>

void kacpi_fail(int line)
{
    kdebug_outf("\r\nkacpi: failed to read ACPI tables! aborting [L:%d]", line);
    for(;;);
}

int kacpi_sdtchecksum(acpi_sdt_header *h)
{
    uint8_t check = 0;
    for (int i = 0; i < h->length; i++)
        check += ((unsigned char*)h)[i];
    return check;
}

void kacpi_processtable(acpi_sdt_header *h)
{
    acpi_sdt_header *header = kmem_page((uintptr_t)h, 0x1000, 0b11);
    kdebug_outf("\r\nkacpi: table %04s", header->signature);
    if (strn_cmp("APIC", header->signature, 4) == 0)
    {
        acpi_madt *madt = (acpi_madt *)header;

        if (kacpi_sdtchecksum((acpi_sdt_header *)header) != 0)
            kacpi_fail(__LINE__);

        kacpi_processapic(madt);
        
        kmem_unpage(header, 0x1000);
    }
    else
        kmem_unpage(header, 0x1000);
}

void kacpi_init()
{
    acpi_rsdp *table = (acpi_rsdp *)kmem_page(k_boottable.rsdp, 0x1000, 0b11);

    kdebug_outf("\nkacpi_i: mapped table %x", table);
    kdebug_outf("\r\nkacpi_i: signature [%8s]", table->signature);
    if (!str_cmp(table->signature, "RSD PTR "))
        kacpi_fail(__LINE__);

    if (k_boottable.acpi_ver == 2)
    {
        size_t rsdp_len = table->length;
        kmem_unpage(table, 0x1000);

        table = kmem_page(k_boottable.rsdp, rsdp_len, 0b11);

        uint32_t rsdp_checksum = table->checksum + table->extended_checksum;
        for (int i = 0; i < 36; i++)
            rsdp_checksum += ((unsigned char *)table)[i];

        if (rsdp_checksum & 0x1)
            kacpi_fail(__LINE__);

        acpi_xsdt *xsdt = kmem_page((uintptr_t)table->xsdt_addr, 0x1000, 0b11);

        if (kacpi_sdtchecksum((acpi_sdt_header *)xsdt) != 0)
            kacpi_fail(__LINE__);

        size_t xsdt_len = xsdt->h.length;
        int xsdt_entries = (xsdt->h.length - sizeof(xsdt->h)) / 8;
    
        kmem_unpage(xsdt, 0x1000);
        xsdt = kmem_page((uintptr_t)table->xsdt_addr, xsdt_len, 0b11);
    
        for (int i = 0; i < xsdt_entries; i++)
        {
            acpi_sdt_header *h = (acpi_sdt_header *)xsdt->other_sdt[i];
            kacpi_processtable(h);
        }
    
        kmem_unpage(xsdt, xsdt_len);
        kmem_unpage(table, rsdp_len);
    }
    else if (k_boottable.acpi_ver == 1)
    {
        uint8_t rsdp_checksum = table->checksum;
        for (int i = 0; i < 20; i++)
            rsdp_checksum += ((unsigned char *)table)[i];

        if (rsdp_checksum & 0x1)
            kacpi_fail(__LINE__);

        acpi_rsdt *rsdt = kmem_page((uintptr_t)table->rsdt_addr, 0x1000, 0b11);

        if (kacpi_sdtchecksum((acpi_sdt_header *)rsdt) != 0)
            kacpi_fail(__LINE__);

        size_t rsdt_len = rsdt->h.length;
        int rsdt_entries = (rsdt->h.length - sizeof(rsdt->h)) / 4;

        kmem_unpage(rsdt, 0x1000);
        rsdt = kmem_page((uintptr_t)table->rsdt_addr, rsdt_len, 0b11);

        for (int i = 0; i < rsdt_entries; i++)
        {
            acpi_sdt_header *h = (acpi_sdt_header *)(uintptr_t)rsdt->other_sdt[i];
            kacpi_processtable(h);
        }

        kmem_unpage(rsdt, rsdt_len);
        kmem_unpage(table, 0x1000);
    }
}

#include <output/screen.h>

void kacpi_shutdown()
{
    kscreen_putf("\nacpi not yet implemented!");
}