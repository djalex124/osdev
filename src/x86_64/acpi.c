#include <acpi.h>
#include <debug.h>
#include <mem.h>
#include <kstring.h>

int kacpi_header_check(struct acpi_sdt_header *sdt)
{
    uint8_t check = 0;
    for (int i = 0; i < sdt->length; i++)
        check += ((uint8_t *)sdt)[i];
    if (check != 0)
        return -1;
    return 0;
}

void kacpi_aml_decode(uint8_t* aml, uint64_t end)
{
    kdebug_outf("\r\nkacpi: aml decoding to %x", end);
    while ((uint64_t)aml < end)
    {
        kdebug_outf("\r\nkacpi: aml [%2x]", *aml);
        switch (*aml)
        {
            case 0x10:  // scope op
                kdebug_outf(" defscope"); // name string : <rootchar namepath> | <prefixpath namepath>
                aml++;
                // rootchar = 0x5C prefixpath = nothing | <'^' prefixpath>
                if (*aml == 0x5C)
                    kdebug_outf("|rootchar");
                else if (*aml == 0x5E)
                {
                    kdebug_outf("|prefixpath");
                    aml++;
                }
                else
                    kdebug_outf("|prefixpath");
                aml++;
                // namepath = nameseg | dualnamepath | multinamepath | nullname
                if (*aml == 0x2E)
                {
                    kdebug_outf("|dualnamepath");
                }
                else if (*aml == 0x2F)
                {
                    kdebug_outf("|multinamepath");
                }
                else if (*aml == 0)
                {
                    kdebug_outf("|nullname");
                }
                else
                {
                    kdebug_outf("|nameseg");
                    aml++;
                    char* nameseg = (char*)aml;
                    kdebug_outf("=%4s", nameseg);
                    aml += 4;
                }
                kdebug_outf("|package_lead_byte=%8b", *aml);
                int byte_count = *aml >> 6;
                while (byte_count)
                {
                    aml++;
                    kdebug_outf("-%2x", *aml);
                    byte_count--;
                }
                aml++;
                if (*aml == 0x5C)
                    kdebug_outf("|rootchar");
                else if (*aml == 0x5E)
                {
                    kdebug_outf("|prefixpath");
                    aml++;
                }
                else
                    kdebug_outf("|prefixpath");
                aml++;
                if (*aml == 0x2E)
                {
                    kdebug_outf("|dualnamepath");
                }
                else if (*aml == 0x2F)
                {
                    kdebug_outf("|multinamepath");
                }
                else if (*aml == 0)
                {
                    kdebug_outf("|nullname");
                }
                else
                {
                    kdebug_outf("|nameseg");
                    aml++;
                    char* nameseg = (char*)aml;
                    kdebug_outf("=%x%x%x%x", (char)nameseg[0], (char)nameseg[1], (char)nameseg[2], (char)nameseg[3]);
                    aml += 4;
                }
                break;
            default:
                kdebug_outf(" %c", *aml);
                break;
        }
        aml++;
    }
}

void kacpi_init(struct mutliboot_acpi_tag *acpi_tag)
{
    kdebug_outf("\r\nkacpi: reading from [0x%x]", (uint64_t)acpi_tag);
    struct acpi_rsdp *table = (struct acpi_rsdp *)acpi_tag->rsdp;

    if (table->revision == 0)
    {
        kdebug_outf("\r\nkacpi: version 1.0");
        uint8_t check = 0;
        int i = 0;
        for (; i < 19; i++)
            check += ((uint8_t *)acpi_tag)[i];
        if (((check + table->checksum) & 0x1))
            goto checksum;
        uint64_t sdt_addr = (uint64_t)table->rsdt_addr;
        struct acpi_rsdt *rsdt = (struct acpi_rsdt *)sdt_addr;
        kmem_page(sdt_addr & 0xFFFFF000, sdt_addr & 0xFFFFF000, 0x1000, 0b11);
        if (kacpi_header_check(&rsdt->h))
            goto checksum;
        int entries = (rsdt->h.length - sizeof(rsdt->h)) / 4;
        kdebug_outf("\r\nkacpi: rsdt [%x] entries [%d]", (uint64_t)rsdt, entries);
        struct acpi_fadt *facp = 0;
        for (i = 0; i < entries; i++)
        {
            struct acpi_sdt_header *sdt = (struct acpi_sdt_header *)(uint64_t)rsdt->other_sdt[i];
            if (!str_cmp(sdt->signature, "FACP", 4))
                facp = (struct acpi_fadt *)(uint64_t)sdt;
        }
        if (kacpi_header_check(&facp->h))
            goto checksum;
        kdebug_outf("\r\nkacpi: facp [%x]", (uint64_t)facp);
        struct acpi_dsdt *dsdt = (struct acpi_dsdt *)(uint64_t)facp->dsdt;
        kmem_page((uint64_t)dsdt & 0xFFFFF000, (uint64_t)dsdt & 0xFFFFF000, 0x1000, 0b11);
        if (kacpi_header_check(&dsdt->h))
            goto checksum;
        kdebug_outf("\r\nkacpi: dsdt [%x]", (uint64_t)dsdt);
        kacpi_aml_decode(dsdt->aml, (uint64_t)dsdt + dsdt->h.length);
    }
    else if (table->revision == 2)
    {
        kdebug_outf("\r\nkacpi: version 2.0+");
    }
    else
    {
        kdebug_outf("\r\nkacpi: invalid version!");
    }

    return;

    checksum:
    kdebug_outf("\r\nkacpi: invalid checksum!");
    return;
}