#include <debug.h>
#include <screen.h>
#include <port.h>
#include <pci.h>
#include <mem.h>

//#define AQUA_IDE_DEBUG

size_t kpci_tablesize = 0;
kpci_headercommon *kpci_table = NULL;

void kpci_checkbus(uint8_t bus);

uint16_t kpci_configreadword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off)
{
    uint32_t addr;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;

    addr = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (off & 0xFC) | ((uint32_t)0x80000000));
    outl(0xCF8, addr);

    tmp = (uint16_t)((inl(0xCFC) >> ((off & 2) * 8)) & 0xFFFF);
    return tmp;
}

uint16_t kpci_getdeviceid(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configreadword(bus, device, func, 0x2);
}

uint16_t kpci_getvendorid(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configreadword(bus, device, func, 0x0);
}

uint8_t kpci_getbaseclass(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configreadword(bus, device, func, 0xB) & 0xFF;
}

uint8_t kpci_getsubclass(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configreadword(bus, device, func, 0xA) & 0xFF;
}

uint8_t kpci_getprogif(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configreadword(bus, device, func, 0x9) & 0xFF;
}

uint8_t kpci_getheadertype(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configreadword(bus, device, func, 0xE) & 0xFF;
}

uint8_t kpci_getsecondarybus(uint8_t bus, uint8_t device, uint8_t func)
{
    return kpci_configreadword(bus, device, func, 0x19) & 0xFF;
}

void kpci_confirmedfunction(uint8_t bus, uint8_t device, uint8_t func)
{
    uint8_t base = kpci_getbaseclass(bus, device, func);
    uint8_t sub  = kpci_getsubclass(bus, device, func);
    uint16_t dev = kpci_getdeviceid(bus, device, func);

    uint16_t vendorid = kpci_getvendorid(bus, device, func);
    uint8_t progif = kpci_getprogif(bus, device, func);

#ifdef AQUA_IDE_DEBUG
    kdebug_outf("\r\nkpci_i: PCI(B%xD%x) F%x ID%x", bus, device, func, dev);
    kdebug_outf(" V%x CLASS %2x:%2x PROGIF %x", vendorid, base, sub, progif);
#endif

    if (kpci_table != NULL)
        kpci_table = kmem_kalloc(sizeof(kpci_headercommon));

    kpci_table[kpci_tablesize].bus = bus;
    kpci_table[kpci_tablesize].device = device;
    kpci_table[kpci_tablesize].function = func;
    
    kpci_table[kpci_tablesize].class = base;
    kpci_table[kpci_tablesize].subclass = sub;
    kpci_table[kpci_tablesize].deviceid = dev;
    kpci_table[kpci_tablesize].progif = progif;
    kpci_table[kpci_tablesize].vendorid = vendorid;
    
    if (kpci_tablesize != 0)
        kmem_kalloc(sizeof(kpci_headercommon));

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

    for (int i = 0; i < kpci_tablesize; i++)
    {
        if (kpci_table[i].class == 0x1 && kpci_table[i].subclass == 0x1)
        {
            kdebug_outf("\r\nkpci_i: IDE controller found B%xD%xF%x",
                kpci_table[i].bus, kpci_table[i].device, kpci_table[i].function);
        }
    }
}

void kpci_printinfo()
{
    kscreen_putf("\nkpci_info: current pci device table");
    for (int i = 0; i < kpci_tablesize; i++)
    {
        kscreen_putf("\n - Bus %2x Device %2x Function %2x", kpci_table[i].bus, kpci_table[i].device, kpci_table[i].function);
        kscreen_putf(": Class %2x/%2x VendorID %x DeviceID %x Prog IF %x", 
            kpci_table[i].class, kpci_table[i].subclass, kpci_table[i].vendorid, kpci_table[i].deviceid, kpci_table[i].progif);
    }
}