#include <debug.h>
#include <screen.h>
#include <port.h>
#include <pci.h>
#include <mem.h>
#include <kernel.h>

#define AQUA_IDE_DEBUG

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
        case 6:
            if (subclass >= 11)
                return kpci_classname[22];
            return kpci_subclassname_6[subclass];
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

size_t kpci_tablesize = 0;
kpci_device *kpci_table = NULL;

void kpci_checkbus(uint8_t bus);

uint32_t kpci_configread(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off)
{
    uint32_t addr;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint32_t tmp = 0;

    addr = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (off & 0xFC) | ((uint32_t)0x80000000));
    outl(0xCF8, addr);

    tmp = inl(0xCFC) >> ((off & 3) * 0x8);
    return tmp;
}

uint16_t kpci_getvendorid(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configread(bus, device, func, PCI_OFFSET_VENDORID) & 0xFFFF;
}

uint8_t kpci_getbaseclass(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configread(bus, device, func, PCI_OFFSET_CLASS) & 0xFF;
}

uint8_t kpci_getsubclass(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configread(bus, device, func, PCI_OFFSET_SUBCLASS) & 0xFF;
}

uint8_t kpci_getheadertype(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configread(bus, device, func, PCI_OFFSET_HDRTYPE) & 0xFF;
}

uint8_t kpci_getsecondarybus(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configread(bus, device, func, 0x19) & 0xFF;
}

void kpci_confirmedfunction(uint8_t bus, uint8_t device, uint8_t func)
{
    uint8_t base = kpci_getbaseclass(bus, device, func);
    uint8_t sub  = kpci_getsubclass(bus, device, func);
    uint16_t ven = kpci_getvendorid(bus, device, func);

#ifdef AQUA_IDE_DEBUG
    kdebug_outf("\r\nkpci_i: PCI(B%xD%x) F%x V%x CLASS %2x:%2x", bus, device, func, ven, base, sub);
#endif

    if (kpci_table != NULL)
        kpci_table = kmem_kalloc(sizeof(kpci_device));

    kpci_table[kpci_tablesize].bus = bus;
    kpci_table[kpci_tablesize].device = device;
    kpci_table[kpci_tablesize].function = func;
    kpci_table[kpci_tablesize].class = base;
    kpci_table[kpci_tablesize].subclass = sub;
    kpci_table[kpci_tablesize].vendorid = ven;
    
    if (kpci_tablesize != 0)
        kmem_kalloc(sizeof(kpci_device));

    kpci_tablesize++;
}

void kpci_checkfunction(uint8_t bus, uint8_t device, uint8_t func)
{
    uint8_t base;
    uint8_t sub;
    uint8_t secondary = 0;

    base = kpci_getbaseclass(bus, device, func);
    sub = kpci_getsubclass(bus, device, func);
    if ((base == 0x6) && (sub == 0x4))
    {
        secondary = kpci_getsecondarybus(bus, device, func);
        kpci_checkbus(secondary);
    }

    kpci_confirmedfunction(bus, device, func);
}

void kpci_checkdevice(uint8_t bus, uint8_t device)
{
    uint8_t function = 0;

    uint16_t vendorid;
    vendorid = kpci_getvendorid(bus, device, function);
    if (vendorid == 0xFFFF) return;
    kpci_checkfunction(bus, device, function);
    uint8_t headertype;
    headertype = kpci_getheadertype(bus, device, function);
    if ((headertype & 0x80) != 0)
    {
        for (function = 1; function < 8; function++)
        {
            if (kpci_getvendorid(bus, device, function) != 0xFFFF)
                kpci_checkfunction(bus, device, function);
        }
    }
}

void kpci_checkbus(uint8_t bus)
{
    uint8_t device;

    for (device = 0; device < 32; device++)
        kpci_checkdevice(bus, device);
}

void kpci_checkall()
{
    uint8_t function;
    uint8_t bus;

    uint8_t headertype;
    headertype = kpci_getheadertype(0, 0, 0);
    if ((headertype & 0x80) == 0)
        kpci_checkbus(0);
    else
    {
        for (function = 0; function < 8; function++)
        {
            if (kpci_getvendorid(0, 0, function) != 0xFFFF)
                break;
            bus = function;
            kpci_checkbus(bus);
        }
    }
}

void kpci_init()
{
    kdebug_outf("\r\nkpci_i: start iterate pci devices");
    kpci_checkall();

    k_infotable.kpci_tablesize = kpci_tablesize;
    k_infotable.kpci_table = kpci_table;
}