#include <kernel/debug.h>

#include <x86_64/pci.h>

#include <mm/mem.h>

#include <fs/fs.h>

typedef struct
{
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t int_status;
    uint32_t int_enable;
    uint32_t cmd;
    uint32_t resv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t sata_status;
    uint32_t sata_control;
    uint32_t sata_error;
    uint32_t sata_active;
    uint32_t ci;
    uint32_t sata_notif;
    uint32_t fbs;
    uint32_t resv1[11];
    uint32_t vendor[4];
} ahci_hbaport;

typedef struct
{
    uint32_t cap;
    uint32_t ghc;
    uint32_t int_status;
    uint32_t port_i;
    uint32_t version;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;

    uint8_t resv[0x74];
    uint8_t vendor[0x60];

    ahci_hbaport ports[];
} ahci_hbareg;

void kfs_satainit(kpci_device *ahci_device)
{
    kdebug_outf("\nkfs_i: sata device detected");
    uint64_t *abar = (void *)(uint64_t)(kpci_configread(ahci_device, PCI_OFFSET_HDR0_BAR5));
    kdebug_outf("\nkfs_i: ABAR at 0x%x", abar);

    kmem_pageentry((uint64_t)abar, (uint64_t)abar, 0x2000, 0b10011);
    // largest abar is 0x1100

    ahci_hbareg *hba_registers = (ahci_hbareg *)abar;
    uint32_t ahci_enable = (hba_registers->ghc >> 31) & 1;
    kdebug_outf("\nkfs_i: AHCI_Enable bit %x", ahci_enable);

    if (ahci_enable == 0)
    {
        kfs_patainit(ahci_device);
        return;
    }

    for (int i = 0; i < 32; i++)
    {
        if (((hba_registers->port_i >> i) & 1) == 0)
            continue;
        
        kdebug_outf("\nkfs_i: slot %d used", i);
    }
}