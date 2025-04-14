#include <stdint.h>
#include <kernel.h>
#include <debug.h>
#include <pci.h>
#include <fs.h>

void kfs_initide(kpci_device* pci_entry)
{
    kdebug_outf("\r\nkfs_i: starting on pci device bus %x device %x function %x",
        pci_entry->bus, pci_entry->device, pci_entry->function);

    uint8_t progif = kpci_configread(pci_entry->bus, pci_entry->device, pci_entry->function, PCI_OFFSET_PROGIF) & 0xFF;

    kdebug_outf("\r\nkfs_i: ");

    if (progif & 0x1)
        kdebug_outf(" primary channel PCI native mode");
    else
        kdebug_outf(" primary channel compatibility mode");
    if (progif & 0x2)
        kdebug_outf(" (can switch)");
    else
        kdebug_outf(" (can't switch)");
    kdebug_outf("\r\nkfs_i: ");
    if (progif & 0x4)
        kdebug_outf(" secondary channel PCI native mode");
    else
        kdebug_outf(" secondary channel compatibility mode");
    if (progif & 0x8)
        kdebug_outf(" (can switch)");
    else
        kdebug_outf(" (can't switch)");
    if (progif & 0x40)
        kdebug_outf(" bus master");
    else
        kdebug_outf(" no DMA support");
}

void kfs_init()
{
    for (size_t i = 0; i < k_infotable.kpci_tablesize; i++)
    {
        if (k_infotable.kpci_table[i].class == 0x1 && k_infotable.kpci_table[i].subclass == 0x1)
            kfs_initide(&k_infotable.kpci_table[i]);
    }
}