#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <output/screen.h>

#include <x86_64/acpi/acpi.h>
#include <x86_64/port.h>
#include <x86_64/pci.h>

#include <mm/mem.h>

uint8_t kpci_pcie = 0;

char* kpci_classname[] =
{
    "Unclassified",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge",
    "Simple Communication Controller",
    "Base System Peripheral",
    "Input Device Controller",
    "Docking Station",
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent Controller",
    "Satelite Communication Controller",
    "Encryption Controller",
    "Signal Processing Controller",
    "Processing Accelerator",
    "Non-Essential Instrumentation",
    "Co-Processor",
    "Unassigned Class",
    "Reserved"
};

char* kpci_subclass0x80 = "Other";

char* kpci_subclassname_0[] = 
{
    "VGA-Compatible",
    "Non-VGA-Compatible"
};

char* kpci_subclassname_1[] =
{
    "SCSI Bus Controller",
    "IDE Controller",
    "Floppy Disk Controller",
    "IPI Bus Controller",
    "RAID Controller",
    "ATA Controller",
    "SATA Controller",
    "Serial Attached SCSI Controller",
    "Non-Volatile Memory Controller"
};

char* kpci_subclassname_2[] =
{
    "Ethernet Controller"
};

char* kpci_subclassname_3[] =
{
    "VGA Compatible Controller",
    "XGA Controller",
    "3D Controller"
};

char* kpci_subclassname_4[] =
{
    "Video Device",
    "Audio Device",
    "Telephone Device",
    "HD Audio"
};

char* kpci_subclassname_6[] =
{
    "Host Bridge",
    "ISA Bridge",
    "EISA Bridge",
    "MCA Bridge",
    "PCI-PCI Bridge",
    "PCMIA Bridge",
    "NuBus Bridge",
    "CardBus Bridge",
    "RACEway Bridge",
    "PCI-PCI Bridge",
    "InfiniBand-PCI Bridge"
};

char* kpci_subclassname_8[] =
{
    "PIC",
    "DMA Controller",
    "Timer",
    "RTC Controller",
    "PCI Hot-Plug",
    "SD Host",
    "IOMMU"
};

char* kpci_subclassname_12[] =
{
    "FireWire Controller",
    "ACCESS Bus Controller",
    "SSA",
    "USB Controller",
    "Fibre Channel",
    "SMBus Controller",
    "InfiniBand Controller",
    "IPMI Interface",
    "SERCOS Interface",
    "CANbus Controller"
};

char* kpci_getsubclassname(uint8_t class, uint8_t subclass)
{
    if (subclass == 0x80)
        return kpci_subclass0x80;
    switch (class)
    {
        case 0:
            if (subclass >= 2)
                return kpci_classname[22];
            return kpci_subclassname_0[subclass];
        case 1:
            if (subclass >= 9)
                return kpci_classname[22];
            return kpci_subclassname_1[subclass];
        case 2:
            if (subclass >= 1)
                return kpci_subclass0x80;
            return kpci_subclassname_2[subclass];
        case 3:
            if (subclass >= 3)
                return kpci_classname[22];
            return kpci_subclassname_3[subclass];
        case 4:
            if (subclass >= 4)
                return kpci_classname[22];
            return kpci_subclassname_4[subclass];
        case 6:
            if (subclass >= 11)
                return kpci_classname[22];
            return kpci_subclassname_6[subclass];
        case 8:
            if (subclass >= 7)
                return kpci_classname[22];
            return kpci_subclassname_8[subclass];
        case 12:
            if (subclass >= 10)
                return kpci_classname[22];
            return kpci_subclassname_12[subclass];
        default:
            return kpci_classname[22];
    }
}

char* kpci_getclassname(uint8_t class)
{
    if (class >= 0x14 && class <= 0x3F)
        return kpci_classname[22];
    else if (class >= 0x41 && class <= 0xFE)
        return kpci_classname[22];
    else if (class == 0x40)
        return kpci_classname[21];
    else
        return kpci_classname[class];
}

void kpci_checkbus(uint16_t section, uint8_t bus);

//different function based on pcie existance
//using pointer set in init

uint16_t kpci_sectionheaderscount = 0;
acpi_mcfg_baa_header *kpci_sectionheaders = 0;

uint32_t (*kpci_configread)(kpci_device *, uint8_t);
void (*kpci_configwrite16)(kpci_device *, uint8_t, uint16_t);

uint32_t kpci_configreaddma(kpci_device *device, uint8_t off)
{
    uint64_t device_offset = ((device->bus * 256) + (device->device * 8) + device->function) * 4096;
    uint64_t bar_offset = 0;
    for (int i = 0; i < kpci_sectionheaderscount; i++)
    {
        if (kpci_sectionheaders[i].pci_grpsegnum == device->section)
            bar_offset = kpci_sectionheaders[i].ecm_baseaddr;
    }
    if (bar_offset == 0)
        kdebug_outf("\nkpci: unable to find section %d?", device->section);
    return *(uint32_t *)(bar_offset + device_offset + off);
}

void kpci_configwrite16dma(kpci_device *device, uint8_t off, uint16_t val)
{
    uint64_t device_offset = ((device->bus * 256) + (device->device * 8) + device->function) * 4096;
    uint64_t bar_offset = 0;
    for (int i = 0; i < kpci_sectionheaderscount; i++)
    {
        if (kpci_sectionheaders[i].pci_grpsegnum == device->section)
            bar_offset = kpci_sectionheaders[i].ecm_baseaddr;
    }
    if (bar_offset == 0)
        kdebug_outf("\nkpci: unable to find section %d?", device->section);
    *(uint16_t *)(bar_offset + device_offset + off) = val;
}

uint32_t kpci_configreadport(kpci_device *device, uint8_t off)
{
    uint32_t addr;
    uint32_t lbus = (uint32_t)device->bus;
    uint32_t lslot = (uint32_t)device->device;
    uint32_t lfunc = (uint32_t)device->function;
    uint32_t tmp = 0;

    addr = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (off & 0xFC) | ((uint32_t)0x80000000));
    outl(0xCF8, addr);

    tmp = inl(0xCFC) >> ((off & 3) * 0x8);
    return tmp;
}

void kpci_configwrite16port(kpci_device *device, uint8_t off, uint16_t val)
{
    uint32_t addr;
    uint32_t lbus = (uint32_t)device->bus;
    uint32_t lslot = (uint32_t)device->device;
    uint32_t lfunc = (uint32_t)device->function;

    addr = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (off & 0xFC) | ((uint32_t)0x80000000));
    outl(0xCF8, addr);

    outw(0xCFC, val);
}

uint16_t kpci_getdeviceid(kpci_device *device)
{
    return kpci_configread(device, PCI_OFFSET_DEVICEID) & 0xFFFF;
}

uint16_t kpci_getvendorid(kpci_device *device)
{
    return kpci_configread(device, PCI_OFFSET_VENDORID) & 0xFFFF;
}

uint8_t kpci_getbaseclass(kpci_device *device)
{
    return kpci_configread(device, PCI_OFFSET_CLASS) & 0xFF;
}

uint8_t kpci_getsubclass(kpci_device *device)
{
    return kpci_configread(device, PCI_OFFSET_SUBCLASS) & 0xFF;
}

uint8_t kpci_getheadertype(kpci_device *device)
{
    return kpci_configread(device, PCI_OFFSET_HDRTYPE) & 0xFF;
}

uint8_t kpci_getsecondarybus(kpci_device *device)
{
    return kpci_configread(device, 0x19) & 0xFF;
}

void kpci_confirmedfunction(uint16_t section, uint8_t bus, uint8_t device, uint8_t func)
{
    kpci_device *new_device = &k_infotable.kpci_table[k_infotable.kpci_tablesize];

    new_device->section = section;
    new_device->bus = bus;
    new_device->device = device;
    new_device->function = func;

#ifdef AQUA_DEBUG
    uint8_t base = kpci_getbaseclass(new_device);
    uint8_t sub  = kpci_getsubclass(new_device);
    uint16_t ven = kpci_getvendorid(new_device);
    uint32_t ints = kpci_configread(new_device, PCI_OFFSET_HDR0_REGF);
    kdebug_outf("\r\nkpci_i: PCI(S%xB%xD%x) F%x V%x CLASS %2x:%2x INT %x:%x", section, bus, device, func, ven, base, sub, (ints & 0xFF00) >> 8, ints & 0xFF);
#endif

    k_infotable.kpci_tablesize++;
}

void kpci_checkfunction(kpci_device *device)
{
    uint8_t base;
    uint8_t sub;
    uint8_t secondary = 0;

    base = kpci_getbaseclass(device);
    sub = kpci_getsubclass(device);
    if ((base == 0x6) && (sub == 0x4))
    {
        secondary = kpci_getsecondarybus(device);
        kpci_checkbus(device->section, secondary);
    }

    kpci_confirmedfunction(device->section, device->bus, device->device, device->function);
}

void kpci_checkdevice(uint16_t section, uint8_t bus, uint8_t device)
{
    kpci_device test_device = {.section = section, .bus = bus, .device = device, .function = 0};

    uint16_t vendorid;
    vendorid = kpci_getvendorid(&test_device);
    if (vendorid == 0xFFFF) return;
    kpci_checkfunction(&test_device);
    uint8_t headertype;
    headertype = kpci_getheadertype(&test_device);
    if ((headertype & 0x80) != 0)
    {
        for (test_device.function = 1; test_device.function < 8; test_device.function++)
        {
            if (kpci_getvendorid(&test_device) != 0xFFFF)
                kpci_checkfunction(&test_device);
        }
    }
}

void kpci_checkbus(uint16_t section, uint8_t bus)
{
    uint8_t device;

    for (device = 0; device < 32; device++)
        kpci_checkdevice(section, bus, device);
}

void kpci_checkall()
{
    kpci_device test_device = {.section = 0, .bus = 0, .device = 0, .function = 0};
    uint8_t headertype;

    if (kpci_pcie == 1)
    {
        for (int s = 0; s < kpci_sectionheaderscount; s++)
        {
            test_device.section = kpci_sectionheaders[s].pci_grpsegnum;
            headertype = kpci_getheadertype(&test_device);
            if ((headertype & 0x80) == 0)
                kpci_checkbus(test_device.section, 0);
            else
            {
                for (test_device.function = 0; test_device.function < 8; test_device.function++)
                {
                    if (kpci_getvendorid(&test_device) != 0xFFFF)
                        break;
                    kpci_checkbus(test_device.section, test_device.function);
                }
            }
        }
    }
    else
    {
        headertype = kpci_getheadertype(&test_device);
        if ((headertype & 0x80) == 0)
            kpci_checkbus(0, 0);
        else
        {
            for (test_device.function = 0; test_device.function < 8; test_device.function++)
            {
                if (kpci_getvendorid(&test_device) != 0xFFFF)
                    break;
                kpci_checkbus(0, test_device.function);
            }
        }
    }
}

void kpci_init()
{
    kdebug_outf("\r\nkpci_i: start iterate pci devices");

    if ((uint64_t)k_infotable.mcfg_table != 0)
    {
        kdebug_outf("\nkpci_i: PCIe detected");
        
        acpi_mcfg *mcfg = (acpi_mcfg *)k_infotable.mcfg_table;
        acpi_mcfg_baa_header *baa = &mcfg->pci_baa[0];

        uint64_t end_addr = (uint64_t)mcfg + mcfg->h.length - 1;
        int i = 0;
        
        for (; (uint64_t)baa < end_addr; i++, baa = &mcfg->pci_baa[i])
        {
            kdebug_outf("\nkpci_i: baa header section %d", baa->pci_grpsegnum);
            kdebug_outf("\nkpci_i:   base addr %x", baa->ecm_baseaddr);
            kdebug_outf("\nkpci_i:   busses %d-%d", baa->pci_busnum, baa->pci_busnumend);

            // identity map every pci bus in config range
            kmem_pageentry(k_ptab4, baa->ecm_baseaddr, baa->ecm_baseaddr, (baa->pci_busnumend + 1) * 32 * 0x1000,
                kmem_paging_present | kmem_paging_writable | kmem_paging_no_cache, kmem_paging_1kb);
        }

        kpci_sectionheaders = kmem_kalloc(sizeof(acpi_mcfg_baa_header) * i);
        memcpy(kpci_sectionheaders, &mcfg->pci_baa[0], sizeof(acpi_mcfg_baa_header) * i);
        kpci_sectionheaderscount = i;

        kpci_configread = kpci_configreaddma;
        kpci_configwrite16 = kpci_configwrite16dma;

        kpci_pcie = 1;
    }
    else
    {
        kpci_configread = kpci_configreadport;
        kpci_configwrite16 = kpci_configwrite16port;
    }

    k_infotable.kpci_table = kmem_alloc(4);
    kpci_checkall();
}