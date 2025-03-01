#include <acpi.h>
#include <debug.h>
#include <mem.h>
#include <kstring.h>

uint8_t* aml = 0;

int kacpi_header_check(struct acpi_sdt_header *sdt)
{
    uint8_t check = 0;
    for (int i = 0; i < sdt->length; i++)
        check += ((uint8_t *)sdt)[i];
    if (check != 0)
        return -1;
    return 0;
}

void kacpi_aml_decode(uint64_t end)
{
    //kdebug_outf("\r\nkacpi: aml decoding to %x", end);
    kdebug_outf("\r\nkacpi: aml decoding temp disabled");
    /*while ((uint64_t)aml < end)
    {
        kdebug_outf("\r\nkacpi: aml [%2x]", *aml);
        switch (*aml)
        {
            case 0x10: // scope op - pkg - namestr - termlist
                kdebug_outf(" DefScope");
                break;
            default:
                kdebug_outf(" %c", *aml);
                break;
        }
        aml++;
    }*/
}

void kacpi_parse_mcfg(struct acpi_mcfg *mcfg)
{
    if (mcfg == 0)
    {
        kdebug_outf("\r\nkacpi: no pci devices given by acpi?");
        return;
    }
    int enteries = (mcfg->h.length - sizeof(mcfg->h) - 8) / 16;
    kdebug_outf("\r\nkacpi: mcfg enteries = %x", enteries);
    struct acpi_mcfg_baa_header *baas = (struct acpi_mcfg_baa_header *)mcfg->pci_baa;
    for (int i = 0; i < enteries; i++)
    {
        kdebug_outf("\r\nkacpi: mcfg[%2d] [%16x][%4x][%2x][%2x]", i, baas[i].ecm_baseaddr, baas[i].pci_grpsegnum, baas[i].pci_busnum,
            baas[i].pci_busnumend);
        uint64_t address = baas[i].ecm_baseaddr + (( baas[i].pci_busnumend - baas[i].pci_busnum) << 20);
        kdebug_outf("\r\nkacpi: mcfg[%2d] addr [%x]", i, address);
    }
}

void kacpi_init()
{
    kdebug_outf("\r\nkacpi: disabled until UEFI boot established well enough");
    /*
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
        uint64_t rsdt_len = rsdt->h.length;
        kmem_unpage(sdt_addr & 0xFFFFF000, 0x1000);
        kmem_page(sdt_addr & 0xFFFFF000, sdt_addr & 0xFFFFF000, rsdt_len, 0b11);
        int entries = (rsdt->h.length - sizeof(rsdt->h)) / 4;
        kdebug_outf("\r\nkacpi: rsdt [%x] entries [%d]", (uint64_t)rsdt, entries);
        struct acpi_fadt *facp = 0;
        struct acpi_mcfg *mcfg = 0;
        for (i = 0; i < entries; i++)
        {
            struct acpi_sdt_header *sdt = (struct acpi_sdt_header *)(uint64_t)rsdt->other_sdt[i];
            if (!str_cmp(sdt->signature, "FACP", 4))
                facp = (struct acpi_fadt *)(uint64_t)sdt;
            if (!str_cmp(sdt->signature, "MCFG", 4))
                mcfg = (struct acpi_mcfg *)(uint64_t)sdt;
        }
        kdebug_outf("\r\nkacpi: facp [%x]", (uint64_t)facp, facp->h.length);
        struct acpi_dsdt *dsdt = (struct acpi_dsdt *)(uint64_t)facp->dsdt;
        kmem_page((uint64_t)dsdt & 0xFFFFF000, (uint64_t)dsdt & 0xFFFFF000, 0x1000, 0b11);
        uint64_t dsdt_len = dsdt->h.length;
        kmem_unpage((uint64_t)dsdt & 0xFFFFF000, 0x1000);
        kmem_page((uint64_t)dsdt & 0xFFFFF000, (uint64_t)dsdt & 0xFFFFF000, dsdt_len, 0b11);
        kdebug_outf("\r\nkacpi: dsdt [%x]", (uint64_t)dsdt, dsdt->h.length);
        aml = dsdt->aml;
        kacpi_aml_decode((uint64_t)dsdt + dsdt->h.length);
        kmem_page((uint64_t)mcfg & 0xFFFFF000, (uint64_t)mcfg & 0xFFFFF000, 0x1000, 0b11);
        kdebug_outf("\r\nkacpi: mcfg [%x]", (uint64_t)mcfg, mcfg->h.length);
        kacpi_parse_mcfg(mcfg);
        kmem_unpage((uint64_t)dsdt & 0xFFFFF000, dsdt_len);
        kmem_unpage((uint64_t)mcfg & 0xFFFFF000, 0x1000);
        kmem_unpage(sdt_addr & 0xFFFFF000, rsdt_len);
    }
    else if (table->revision == 2)
    {
        kdebug_outf("\r\nkacpi: version 2.0+");
        uint8_t check = 0;
        int i = 0;
        for (; i < 19; i++)
            check += ((uint8_t *)acpi_tag)[i];
        if (((check + table->checksum) & 0x1))
            goto checksum;
        check = 0;
        for (; i < 35; i++)
            check += ((uint8_t *)acpi_tag)[i];
        if (((check + table->checksum) & 0x1))
            goto checksum;
    }
    else
    {
        kdebug_outf("\r\nkacpi: invalid version!");
    }

    return;

    checksum:
    kdebug_outf("\r\nkacpi: invalid checksum!");
    */
}