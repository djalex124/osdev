#include <acpi.h>
#include <debug.h>
#include <mem.h>
#include <kstring.h>

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
    kmem_page((uintptr_t)h & 0xFFFFF000, (uintptr_t)h & 0xFFFFF000, 0x1000, 0b11);
    kdebug_outf("\r\nkacpi: table %04s", h->signature);
    if (strn_cmp("APIC", h->signature, 4) == 0)
    {
        acpi_madt *madt = (acpi_madt *)h;

        if (kacpi_sdtchecksum((acpi_sdt_header *)h) != 0)
            kacpi_fail(__LINE__);

        kacpi_processapic(madt);
        
        kmem_unpage((uintptr_t)h & 0xFFFFF000, 0x1000);
    }
    else if (strn_cmp("FACP", h->signature, 4) == 0)
    {
        acpi_fadt *fadt = (acpi_fadt *)h;
        kdebug_outf("\r\nkacpi: processing FADT table");

        if (kacpi_sdtchecksum((acpi_sdt_header *)h) != 0)
            kacpi_fail(__LINE__);
        
        kdebug_outf("\r\n - dsdt %x", fadt->dsdt);

        acpi_dsdt *dsdt = (acpi_dsdt *)(uint64_t)fadt->dsdt;
        kmem_page((uintptr_t)dsdt & 0xFFFFF000, (uintptr_t)dsdt & 0xFFFFF000, 0x1000, 0b11);

        uint64_t dsdt_len = dsdt->h.length;
        kmem_unpage((uintptr_t)dsdt & 0xFFFFF000, 0x1000);
        kmem_page((uintptr_t)dsdt & 0xFFFFF000, (uintptr_t)dsdt & 0xFFFFF000, dsdt_len, 0b11);

        if (strn_cmp("DSDT", dsdt->h.signature, 4) != 0)
            kacpi_fail(__LINE__);
        else if (kacpi_sdtchecksum((acpi_sdt_header *)dsdt) != 0)
            kacpi_fail(__LINE__);

        kacpi_processdsdt(dsdt);
        
        kmem_unpage((uintptr_t)dsdt & 0xFFFFF000, dsdt_len);

        kmem_unpage((uintptr_t)h & 0xFFFFF000, 0x1000);
    }
    else
        kmem_unpage((uintptr_t)h & 0xFFFFF000, 0x1000);
}

void kacpi_init()
{
    kmem_page(k_boottable.rsdp & 0xFFFFF000, k_boottable.rsdp & 0xFFFFF000, 0x1000, 0b11);
    acpi_rsdp *table = (acpi_rsdp *)k_boottable.rsdp;

    kdebug_outf("\r\nkacpi_i: signature [%8s]", table->signature);
    if (!str_cmp(table->signature, "RSD PTR "))
        kacpi_fail(__LINE__);

    if (k_boottable.acpi_ver == 2)
    {
        size_t rsdp_len = table->length;
        kmem_unpage((uintptr_t)table & 0xFFFFF000, 0x1000);

        kmem_page((uintptr_t)table & 0xFFFFF000, (uintptr_t)table & 0xFFFFF000, rsdp_len, 0b11);

        uint32_t rsdp_checksum = table->checksum + table->extended_checksum;
        for (int i = 0; i < 36; i++)
            rsdp_checksum += ((unsigned char *)table)[i];

        if (rsdp_checksum & 0x1)
            kacpi_fail(__LINE__);

        acpi_xsdt *xsdt = (acpi_xsdt *)table->xsdt_addr;
        kmem_page((uintptr_t)xsdt & 0xFFFFF000, (uintptr_t)xsdt & 0xFFFFF000, 0x1000, 0b11);

        if (kacpi_sdtchecksum((acpi_sdt_header *)xsdt) != 0)
            kacpi_fail(__LINE__);

        size_t xsdt_len = xsdt->h.length;
        int xsdt_entries = (xsdt->h.length - sizeof(xsdt->h)) / 8;
    
        kmem_unpage((uintptr_t)xsdt & 0xFFFFF000, 0x1000);
        kmem_page((uintptr_t)xsdt & 0xFFFFF000, (uintptr_t)xsdt & 0xFFFFF000, xsdt_len, 0b11);
    
        for (int i = 0; i < xsdt_entries; i++)
        {
            acpi_sdt_header *h = (acpi_sdt_header *)xsdt->other_sdt[i];
            kacpi_processtable(h);
        }
    
        kmem_unpage((uintptr_t)xsdt & 0xFFFFF000, xsdt_len);
        kmem_unpage((uintptr_t)table & 0xFFFFF000, rsdp_len);
    }
    else if (k_boottable.acpi_ver == 1)
    {
        uint8_t rsdp_checksum = table->checksum;
        for (int i = 0; i < 20; i++)
            rsdp_checksum += ((unsigned char *)table)[i];

        if (rsdp_checksum & 0x1)
            kacpi_fail(__LINE__);

        acpi_rsdt *rsdt = (acpi_rsdt *)(uintptr_t)table->rsdt_addr;
        kmem_page((uintptr_t)rsdt & 0xFFFFF000, (uintptr_t)rsdt & 0xFFFFF000, 0x1000, 0b11);

        if (kacpi_sdtchecksum((acpi_sdt_header *)rsdt) != 0)
            kacpi_fail(__LINE__);

        size_t rsdt_len = rsdt->h.length;
        int rsdt_entries = (rsdt->h.length - sizeof(rsdt->h)) / 4;

        kmem_unpage((uintptr_t)rsdt & 0xFFFFF000, 0x1000);
        kmem_page((uintptr_t)rsdt & 0xFFFFF000, (uintptr_t)rsdt & 0xFFFFF000, rsdt_len, 0b11);

        for (int i = 0; i < rsdt_entries; i++)
        {
            acpi_sdt_header *h = (acpi_sdt_header *)(uintptr_t)rsdt->other_sdt[i];
            kacpi_processtable(h);
        }

        kmem_unpage((uintptr_t)rsdt & 0xFFFFF000, rsdt_len);
        kmem_unpage((uintptr_t)table & 0xFFFFF000, 0x1000);
    }
}