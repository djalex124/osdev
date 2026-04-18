#define sata_file

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <x86_64/pci.h>

#include <mm/mem.h>

#include <fs/fs_ata.h>
#include <fs/fs.h>

typedef struct
{
    uint8_t cflen:5;
    uint8_t atapi:1;
    uint8_t rw:1;
    uint8_t prefetch:1;

    uint8_t reset:1;
    uint8_t bist:1;
    uint8_t c:1;
    uint8_t resv0:1;
    uint8_t pmp:4;

    uint16_t prdtl;

    uint32_t prdbc;

    uint32_t ctba;
    uint32_t ctba_upper;

    uint32_t resv1[4];
}__attribute__((packed)) ahci_cmdheader;

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
}__attribute__((packed)) ahci_hbaport;

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
}__attribute__((packed)) ahci_hbareg;

uint64_t ahci_base = 0;

void kfs_satarebase(ahci_hbaport *port, int portnum)
{
    port->cmd &= ~0x01;
    port->cmd &= ~0x10;

    while (1)
    {
        if (port->cmd & 0x4000)
            continue;
        if (port->cmd & 0x8000)
            continue;
        break;
    }

    port->clb = ahci_base + (portnum << 10);
    port->clbu = 0;
    memset((void *)(uint64_t)port->clb, 0, 1024);

    port->fb = ahci_base + (32 << 10) + (portnum << 8);
    port->fbu = 0;
    memset((void *)(uint64_t)port->fb, 0, 256);

    ahci_cmdheader *cmd = (ahci_cmdheader *)(uint64_t)port->clb;
    for (int i = 0; i < 32; i++)
    {
        cmd[i].prdtl = 8;
        cmd[i].ctba = ahci_base + (40 << 10) + (portnum << 13) + (i << 8);
        cmd[i].ctba_upper = 0;
        memset((void *)(uint64_t)cmd[i].ctba, 0, 256);
    }

    kdebug_outf(" test");

    while (port->cmd & 0x8000)
        __asm__("hlt");

    port->cmd |= 0x10;
    port->cmd |= 0x01;
}

void kfs_satainit(kpci_device *ahci_device)
{
    kdebug_outf("\nkfs_i: sata device detected");
    uint32_t *abar = (uint32_t *)(uint64_t)(kpci_configread(ahci_device, PCI_OFFSET_HDR0_BAR5));
    kdebug_outf("\nkfs_i: ABAR at 0x%x", abar);

    kmem_pageentry((uint64_t)abar, (uint64_t)abar, 0x2000, 
        kmem_paging_present | kmem_paging_writable | kmem_paging_no_cache, kmem_paging_1kb);
    // largest abar is 0x1100

    ahci_hbareg *hba_registers = (ahci_hbareg *)abar;
    uint32_t ahci_enable = (hba_registers->ghc >> 31) & 1;
    kdebug_outf("\nkfs_i: AHCI_Enable bit %x", ahci_enable);
    kdebug_outf("\nkfs_i: slots %b", hba_registers->port_i);

    if (ahci_enable == 0)
    {
        kfs_patainit(ahci_device);
        return;
    }

#ifdef AQUA_DEBUG
    uint32_t ints = kpci_configread(ahci_device, PCI_OFFSET_HDR0_REGF);
    kdebug_outf("\r\nkfs_i: int pin: %x int line: %x", (ints & 0xFF00) >> 8, ints & 0xFF);
#endif

    ahci_base = (uint64_t)kmem_palloc(76, kmem_paging_1kb);
    kmem_unpageentry(ahci_base, 76 * 0x1000);
    kmem_pageentry(ahci_base, ahci_base, 76 * 0x1000,
        kmem_paging_present | kmem_paging_writable | kmem_paging_no_cache, kmem_paging_1kb);

    for (int i = 0; i < 32; i++)
    {
        if (((hba_registers->port_i >> i) & 1) == 0)
            continue;
        
        kdebug_outf("\nkfs_i: slot %d used -", i);
        uint32_t present = hba_registers->ports[i].sata_status & 0x0F;
        uint32_t active = (hba_registers->ports[i].sata_status >> 8) & 0x0F;

        if (present)
            kdebug_outf(" present");
        if (active)
            kdebug_outf(" active");

        if (!(present & active))
        {
            kdebug_outf(" no drive");
            continue;
        }

        kfs_satadrive *drive_info = kmem_kalloc(sizeof(kfs_satadrive));
        drive_info->ahci_controller = ahci_device;
        drive_info->drive = i;

        switch (hba_registers->ports[i].sig)
        {
            case 0xEB140101:
                kdebug_outf(" ATAPI drive");
                drive_info->type = AHCI_ATAPI;
                break;
            case 0xC33C0101:
                kdebug_outf(" SEMB drive");
                drive_info->type = AHCI_SEMB;
                continue;
            case 0x96690101:
                kdebug_outf(" PM drive");
                drive_info->type = AHCI_PM;
                continue;
            default:
                kdebug_outf(" ATA drive");
                drive_info->type = AHCI_ATA;
                break;
        }

        kfs_satarebase(&hba_registers->ports[i], i);
        kdebug_outf(" CLB%x FB%x", hba_registers->ports[i].clb, hba_registers->ports[i].fb);

        kfs_drive *drive = kmem_kalloc(sizeof(kfs_drive));
        drive->drive_data = drive_info;
        drive->drive_type = KFS_SATA;
        
        kfs_adddrive(drive);
    }
}