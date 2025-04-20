#include <kstring.h>
#include <debug.h>
#include <port.h>
#include <mem.h>
#include <pci.h>
#include <pit.h>
#include <fs.h>

#define PATA_PRIMARY   0
#define PATA_SECONDARY 1

#define PATA_BASE_PRIMARY   0x1F0
#define PATA_CTRL_PRIMARY   0x3F6
#define PATA_BASE_SECONDARY 0x170
#define PATA_CTRL_SECONDARY 0x376

#define PATA_STATUS_BUSY  0x80
#define PATA_STATUS_DRQ   0x08
#define PATA_STATUS_ERROR 0x1

#define PATA_IDENT_SIGNATURE    0
#define PATA_IDENT_MODEL        54
#define PATA_IDENT_CAPABILITIES 98
#define PATA_IDENT_MAX_LBA      120
#define PATA_IDENT_COMMANDSETS  164
#define PATA_IDENT_MAX_LBA_EXT  200

#define PATA_REG_DATA     0
#define PATA_REG_ERROR    1
#define PATA_REG_FEATURES 1
#define PATA_REG_SECCNT0  2
#define PATA_REG_LBA0     3
#define PATA_REG_LBA1     4
#define PATA_REG_LBA2     5
#define PATA_REG_HDDEVSEL 6
#define PATA_REG_COMMAND  7
#define PATA_REG_STATUS   7
#define PATA_REG_SECCNT1  8
#define PATA_REG_LBA3     9
#define PATA_REG_LBA4     10
#define PATA_REG_LBA5     11
#define PATA_REG_CONTROL  12
#define PATA_REG_ASTATUS  12

kfs_patadrive kfs_patadrives[4];

struct kfs_patachannel {
    uint16_t base;
    uint16_t ctrl;
    uint16_t bmide;
} kfs_channel[2];

void kfs_atawrite(uint8_t c, uint8_t reg, uint8_t v)
{
    if (reg < 0x8)
        outb(kfs_channel[c].base + reg, v);
    else if (reg < 0xC)
        outb(kfs_channel[c].base + reg - 0x6, v);
    else if (reg < 0xE)
        outb(kfs_channel[c].ctrl + reg - 0xA, v);
    else if (reg < 0x16)
        outb(kfs_channel[c].bmide + reg - 0xE, v);
}

uint8_t kfs_ataread(uint8_t c, uint8_t reg)
{
    uint8_t v = 0;
    if (reg < 0x8)
        v = inb(kfs_channel[c].base + reg);
    else if (reg < 0xC)
        v = inb(kfs_channel[c].base + reg - 0x6);
    else if (reg < 0xE)
        v = inb(kfs_channel[c].ctrl + reg - 0xA);
    else if (reg < 0x16)
        v = inb(kfs_channel[c].bmide + reg - 0xE);
    return v;
}

void kfs_atainit(kpci_device *ide_device)
{
    uint8_t progif = kpci_configread(ide_device->bus, ide_device->device, ide_device->function, PCI_OFFSET_PROGIF) & 0xFF;

#ifdef AQUA_DEBUG

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
    if (progif & 0x80)
        kdebug_outf(" bus master");
    else
        kdebug_outf(" no DMA support");

#endif
    
    if (!(progif & 0x80))
    {
        kdebug_outf("\r\nkfs_i: only working with DMA enabled for ATA currently");
        return;
    }

    uint32_t bar4 = kpci_configread(ide_device->bus, ide_device->device, ide_device->function, PCI_OFFSET_HDR0_BAR4);

    kfs_channel[PATA_PRIMARY].base = PATA_BASE_PRIMARY;
    kfs_channel[PATA_SECONDARY].base = PATA_BASE_SECONDARY;
    kfs_channel[PATA_PRIMARY].ctrl = PATA_CTRL_PRIMARY;
    kfs_channel[PATA_SECONDARY].ctrl = PATA_CTRL_SECONDARY;
    kfs_channel[PATA_PRIMARY].bmide = bar4;
    kfs_channel[PATA_SECONDARY].bmide = bar4 + 8;

    kfs_atawrite(PATA_PRIMARY, PATA_REG_CONTROL, 2);
    kfs_atawrite(PATA_SECONDARY, PATA_REG_CONTROL, 2);

    uint8_t packet, status, error = 0, count = 0;

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            kfs_patadrives[count].exists = 0;
            
            kfs_atawrite(i, PATA_REG_HDDEVSEL, 0xA0 | (j << 4));
            ksleep(1);

            kfs_atawrite(i, PATA_REG_COMMAND, 0xEC);
            ksleep(1);

            if (kfs_ataread(i, PATA_REG_STATUS) == 0)
                continue;

            while (1)
            {
                status = kfs_ataread(i, PATA_REG_STATUS);
                if (status & PATA_STATUS_ERROR)
                {
                    error = 1;
                    break;
                }
                if (!(status & PATA_STATUS_BUSY) && (status & PATA_STATUS_DRQ))
                    break;
            }

            if (error != 0)
            {
                uint8_t a, b;
                a = kfs_ataread(i, PATA_REG_LBA1);
                b = kfs_ataread(i, PATA_REG_LBA2);

                if (a == 0x14 && b == 0xEB)
                    packet = 1;
                else if (a == 0x69 && b == 0x96)
                    packet = 1;
                else
                    continue;
                
                kfs_atawrite(i, PATA_REG_COMMAND, 0xA1);
                ksleep(1);
            }

            uint32_t *buffer = kmem_alloc(2);

            for (int n = 0; n < 128; n++)
                buffer[n] = inl(kfs_channel[i].base);

            uint8_t *read_buffer = (uint8_t *)buffer;

            kfs_patadrives[count].exists = 1;
            kfs_patadrives[count].type = packet;
            kfs_patadrives[count].channel = i;
            kfs_patadrives[count].drive = j;
            kfs_patadrives[count].signature = *(uint16_t *)(read_buffer + PATA_IDENT_SIGNATURE);
            kfs_patadrives[count].capabilities = *(uint16_t *)(read_buffer + PATA_IDENT_CAPABILITIES);
            kfs_patadrives[count].commandsets = *(uint32_t *)(read_buffer + PATA_IDENT_COMMANDSETS);

            if (kfs_patadrives[count].commandsets & (1 << 26))
                kfs_patadrives[count].sectors = *(uint32_t *)(read_buffer + PATA_IDENT_MAX_LBA_EXT);
            else
                kfs_patadrives[count].sectors = *(uint32_t *)(read_buffer + PATA_IDENT_MAX_LBA);

            for (int n = 0; n < 40; n += 2)
            {
                kfs_patadrives[count].model[n] = read_buffer[PATA_IDENT_MODEL + n + 1];
                kfs_patadrives[count].model[n + 1] = read_buffer[PATA_IDENT_MODEL + n];
            }
            kfs_patadrives[count].model[40] = 0;

            kfs_atawrite(i, PATA_REG_COMMAND, 0xEC);
            ksleep(1);

            for (int n = 0; n < 128; n++)
                buffer[n] = inl(kfs_channel[i].base);

            uint16_t size_check = *(uint16_t *)(read_buffer + 106);
            kdebug_outf("\r\nkfs_i: size check field %16b", size_check);
            if (((size_check & (1 << 15)) == 0) && ((size_check & (1 << 14))))
            {//check for valid field
                kdebug_outf("\r\nkfs_i: valid size field? find real sector size");
            }
            else
            {
                kfs_patadrives[count].sector_size = 512;
            }

            kmem_free(buffer, 2);

            count++;
        }
    }

#ifdef AQUA_DEBUG
    for (int i = 0; i < 4; i++)
    {
        if (kfs_patadrives[i].exists)
        {
            uint64_t size = kfs_patadrives[i].sectors * kfs_patadrives[i].sector_size;
            kdebug_outf("\r\nkfs_i: pata device %d named '%s' size %d mb", i, 
                kfs_patadrives[i].model, size / 1024 / 1024);
        }
    }
#endif

}