#include <x86_64/acpi/acpi.h>
#include <x86_64/acpi/apic.h>
#include <x86_64/acpi/aml.h>

#include <x86_64/port.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/crash.h>
#include <kernel/debug.h>

#include <mm/mem.h>

char kacpi_crashmessage[35];

void kacpi_fail(int line)
{
    memcpy(kacpi_crashmessage, "Failed to read ACPI tables! L:", 30);
    char *linestr = str_itoa(line, 10);
    memcpy(kacpi_crashmessage + 30, linestr, str_len(linestr));

    kcrash(kacpi_crashmessage);
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
    acpi_sdt_header *header = (acpi_sdt_header *)virt_from_phys((uint64_t)h);
    kdebug_outf("\nkacpi: table %04s at %x", header->signature, (uint64_t)h);
    if (strn_cmp("APIC", header->signature, 4) == 0)
    {
        acpi_madt *madt = (acpi_madt *)header;

        if (kacpi_sdtchecksum((acpi_sdt_header *)header) != 0)
            kacpi_fail(__LINE__);

        kacpi_processapic(madt);
    }
    else if (strn_cmp("FACP", header->signature, 4) == 0)
    {
        acpi_fadt *fadt = (acpi_fadt *)header;

        if (kacpi_sdtchecksum((acpi_sdt_header *)header) != 0)
            kacpi_fail(__LINE__);

        kacpi_processdsdt(fadt->dsdt);
    }
    else if (strn_cmp("MCFG", header->signature, 4) == 0)
    {
        acpi_mcfg *mcfg = (acpi_mcfg *)header;

        if (kacpi_sdtchecksum((acpi_sdt_header *)header) != 0)
            kacpi_fail(__LINE__);

        k_infotable.mcfg_table = (uint64_t *)mcfg;
    }
}

void kacpi_init()
{
    acpi_rsdp *table = (acpi_rsdp *)virt_from_phys(k_boottable.rsdp);

    kdebug_outf("\nkacpi_i: signature [%8s]", table->signature);
    if (!str_cmp(table->signature, "RSD PTR "))
        kacpi_fail(__LINE__);

    if (k_boottable.acpi_ver == 2)
    {
        uint32_t rsdp_checksum = table->checksum + table->extended_checksum;
        for (int i = 0; i < 36; i++)
            rsdp_checksum += ((unsigned char *)table)[i];

        if (rsdp_checksum & 0x1)
            kacpi_fail(__LINE__);

        acpi_xsdt *xsdt = (acpi_xsdt *)virt_from_phys(table->xsdt_addr);

        if (kacpi_sdtchecksum((acpi_sdt_header *)xsdt) != 0)
            kacpi_fail(__LINE__);

        int xsdt_entries = (xsdt->h.length - sizeof(xsdt->h)) / 8;
    
        for (int i = 0; i < xsdt_entries; i++)
        {
            acpi_sdt_header *h = (acpi_sdt_header *)xsdt->other_sdt[i];
            kacpi_processtable(h);
        }
    }
    else if (k_boottable.acpi_ver == 1)
    {
        uint8_t rsdp_checksum = table->checksum;
        for (int i = 0; i < 20; i++)
            rsdp_checksum += ((unsigned char *)table)[i];

        if (rsdp_checksum & 0x1)
            kacpi_fail(__LINE__);

        acpi_rsdt *rsdt = (acpi_rsdt *)virt_from_phys(table->rsdt_addr);

        if (kacpi_sdtchecksum((acpi_sdt_header *)rsdt) != 0)
            kacpi_fail(__LINE__);

        int rsdt_entries = (rsdt->h.length - sizeof(rsdt->h)) / 4;

        for (int i = 0; i < rsdt_entries; i++)
        {
            acpi_sdt_header *h = (acpi_sdt_header *)(uintptr_t)rsdt->other_sdt[i];
            kacpi_processtable(h);
        }
    }
}

#include <output/kterm.h>

void kacpi_shutdown()
{
    //get SLP_TYPa from AML \_S5 object
    kterm_putf("\nacpi not yet implemented!");
}