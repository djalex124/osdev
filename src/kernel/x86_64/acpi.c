#include <acpi.h>
#include <debug.h>
#include <mem.h>
#include <kstring.h>

void kacpi_fail()
{
    kdebug_outf("\r\nkacpi: failed to read ACPI tables! aborting");
    for(;;);
}

int kacpi_sdtchecksum(acpi_sdt_header *h)
{
    uint8_t check = 0;
    for (int i = 0; i < h->length; i++)
        check += ((unsigned char*)h)[i];
    return check;
}

void kacpi_init()
{
    kdebug_outf("\r\nkacpi: rsdp at 0x%x", k_boottable.rsdp);
    
    kmem_page(k_boottable.rsdp & 0xFFFFF000, k_boottable.rsdp & 0xFFFFF000, 0x1000, 0b11);
    acpi_rsdp *table = (acpi_rsdp *)k_boottable.rsdp;

    kdebug_outf("\r\nkacpi: version %d signature %8s", k_boottable.acpi_ver, table->signature);

    if (!str_cmp(table->signature, "RSD PTR "))
        kacpi_fail();

    if (k_boottable.acpi_ver == 2)
    {
        size_t rsdp_len = table->length;
        kmem_unpage((uintptr_t)table & 0xFFFFF000, 0x1000);

        kdebug_outf("\r\nkacpi: rsdp_len = 0x%x", rsdp_len);
        kmem_page((uintptr_t)table & 0xFFFFF000, (uintptr_t)table & 0xFFFFF000, rsdp_len, 0b11);

        uint8_t rsdp_checksum = table->checksum;
        for (int i = 0; i < 36; i++)
            rsdp_checksum += ((unsigned char *)table)[i];

        if (rsdp_checksum & 0x1)
            kacpi_fail();

        acpi_xsdt *xsdt = (acpi_xsdt *)table->xsdt_addr;
        kmem_page((uintptr_t)xsdt & 0xFFFFF000, (uintptr_t)xsdt & 0xFFFFF000, 0x1000, 0b11);

        if (kacpi_sdtchecksum((acpi_sdt_header *)xsdt) != 0)
            kacpi_fail();

        size_t xsdt_len = xsdt->h.length;
        int xsdt_entries = (xsdt->h.length - sizeof(xsdt->h)) / 8;
        kdebug_outf("\r\nkacpi: xsdt_len = 0x%x xsdt_enteries = 0x%x\r\nkacpi: tables", xsdt_len, xsdt_entries);
    
        kmem_unpage((uintptr_t)xsdt & 0xFFFFF000, 0x1000);
        kmem_page((uintptr_t)xsdt & 0xFFFFF000, (uintptr_t)xsdt & 0xFFFFF000, xsdt_len, 0b11);
    
        for (int i = 0; i < xsdt_entries; i++)
        {
            acpi_sdt_header *h = (acpi_sdt_header *)xsdt->other_sdt[i];
            kmem_page((uintptr_t)h & 0xFFFFF000, (uintptr_t)h & 0xFFFFF000, 0x1000, 0b11);
            kdebug_outf(" %04s", h->signature);
            kmem_unpage((uintptr_t)h & 0xFFFFF000, 0x1000);
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
            kacpi_fail();

        acpi_rsdt *rsdt = (acpi_rsdt *)(uintptr_t)table->rsdt_addr;
        kmem_page((uintptr_t)rsdt & 0xFFFFF000, (uintptr_t)rsdt & 0xFFFFF000, 0x1000, 0b11);

        if (kacpi_sdtchecksum((acpi_sdt_header *)rsdt) != 0)
            kacpi_fail();

        size_t rsdt_len = rsdt->h.length;
        int rsdt_entries = (rsdt->h.length - sizeof(rsdt->h)) / 4;
        kdebug_outf("\r\nkacpi: rsdt_len = 0x%x rsdt_enteries = 0x%x\r\nkacpi: tables", rsdt_len, rsdt_entries);

        kmem_unpage((uintptr_t)rsdt & 0xFFFFF000, 0x1000);
        kmem_page((uintptr_t)rsdt & 0xFFFFF000, (uintptr_t)rsdt & 0xFFFFF000, rsdt_len, 0b11);

        for (int i = 0; i < rsdt_entries; i++)
        {
            acpi_sdt_header *h = (acpi_sdt_header *)(uintptr_t)rsdt->other_sdt[i];
            kmem_page((uintptr_t)h & 0xFFFFF000, (uintptr_t)h & 0xFFFFF000, 0x1000, 0b11);
            kdebug_outf(" %04s", h->signature);
            kmem_unpage((uintptr_t)h & 0xFFFFF000, 0x1000);
        }

        kmem_unpage((uintptr_t)rsdt & 0xFFFFF000, rsdt_len);
        kmem_unpage((uintptr_t)table & 0xFFFFF000, 0x1000);
    }
}