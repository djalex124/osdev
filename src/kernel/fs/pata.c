#define pata_file

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <x86_64/desc.h>
#include <x86_64/port.h>
#include <x86_64/pci.h>
#include <x86_64/pit.h>

#include <mm/mem.h>

#include <fs/fs_ata.h>
#include <fs/fs.h>

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

struct kfs_patachannel {
    uint16_t base;
    uint16_t ctrl;
    uint32_t bmide;
    uint8_t  no_int;
    kpci_device *pci;
} kfs_channel[2];

kfs_patadrive kfs_patadrives[4];

void kfs_patawrite(uint8_t c, uint8_t reg, uint8_t v)
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

uint8_t kfs_pataread(uint8_t c, uint8_t reg)
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

uint8_t kfs_ataint[2] = {0, 0};

void kfs_patainterrupt1()
{
    inb(kfs_channel[0].bmide + 2);
    outb(kfs_channel[0].bmide, inb(kfs_channel[0].bmide) & ~1);
    kfs_ataint[0] = 1;
}

void kfs_patainterrupt2()
{
    inb(kfs_channel[1].bmide + 2);
    outb(kfs_channel[1].bmide, inb(kfs_channel[1].bmide) & ~1);
    kfs_ataint[1] = 1;
}

uint8_t *kfs_patadmabuffer = 0;
uint64_t *kfs_prdt = 0;

void kfs_patainit(kpci_device *ide_device)
{
    kfs_patadmabuffer = kmem_palloc(8, kmem_paging_1kb);
    kmem_pageentry((uint64_t)kfs_patadmabuffer, (uint64_t)kfs_patadmabuffer, 0x8000,
        kmem_paging_present | kmem_paging_writable | kmem_paging_no_cache, kmem_paging_1kb);

    kfs_prdt = kmem_palloc(1, kmem_paging_1kb);
    kmem_pageentry((uint64_t)kfs_prdt, (uint64_t)kfs_prdt, 0x1000,
        kmem_paging_present | kmem_paging_writable | kmem_paging_no_cache, kmem_paging_1kb);
    
    uint8_t progif = kpci_configread(ide_device, PCI_OFFSET_PROGIF) & 0xFF;

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
    kdebug_outf("\r\nkfs_i: ");
    if (progif & 0x80)
        kdebug_outf(" bus master");
    else
        kdebug_outf(" no DMA support");

#endif
    
    if (!(progif & 0x80))
    {
        kdebug_outf("\nkfs_i: only using DMA enabled PATA drives, skipping...");
        return;
    }

    uint32_t bar4 = kpci_configread(ide_device, PCI_OFFSET_HDR0_BAR4);

    kfs_channel[ATA_PRIMARY].base = ATA_BASE_PRIMARY;
    kfs_channel[ATA_SECONDARY].base = ATA_BASE_SECONDARY;
    kfs_channel[ATA_PRIMARY].ctrl = ATA_CTRL_PRIMARY;
    kfs_channel[ATA_SECONDARY].ctrl = ATA_CTRL_SECONDARY;
    kfs_channel[ATA_PRIMARY].bmide = bar4 & 0xFFFFFFFC;
    kfs_channel[ATA_SECONDARY].bmide = (bar4 & 0xFFFFFFFC) + 8;

    outb(kfs_channel[ATA_PRIMARY].ctrl, 4);
    outb(kfs_channel[ATA_SECONDARY].ctrl, 4);
    ksleep(1);

    outb(kfs_channel[ATA_PRIMARY].ctrl, 0);
    outb(kfs_channel[ATA_SECONDARY].ctrl, 0);
    ksleep(2);

    kfs_pataread(ATA_PRIMARY, PATA_REG_ERROR);
    kfs_pataread(ATA_SECONDARY, PATA_REG_ERROR);

    kfs_patawrite(ATA_PRIMARY, PATA_REG_CONTROL, 2);
    kfs_patawrite(ATA_PRIMARY, PATA_REG_CONTROL, 2);
    ksleep(5);

    uint8_t packet = 0, status = 0, error = 0, count = 0;

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            kfs_patadrives[count].exists = 0;
            
            kfs_patawrite(i, PATA_REG_HDDEVSEL, 0xA0 | (j << 4));
            ksleep(1);

            kfs_patawrite(i, PATA_REG_SECCNT0, 0);
            kfs_patawrite(i, PATA_REG_LBA0, 0);
            kfs_patawrite(i, PATA_REG_LBA1, 0);
            kfs_patawrite(i, PATA_REG_LBA2, 0);

            kfs_patawrite(i, PATA_REG_COMMAND, ATA_CMD_IDENT);
            ksleep(1);

            if (kfs_pataread(i, PATA_REG_STATUS) == 0)
                continue;

            while (1)
            {
                status = kfs_pataread(i, PATA_REG_STATUS);
                if (status & ATA_STATUS_ERROR)
                {
                    error = 1;
                    break;
                }
                if (!(status & ATA_STATUS_BUSY) && (status & ATA_STATUS_DRQ))
                    break;
            }

            if (error != 0)
            {
                uint8_t a, b;
                a = kfs_pataread(i, PATA_REG_LBA1);
                b = kfs_pataread(i, PATA_REG_LBA2);

                if (a == 0x14 && b == 0xEB)
                    packet = 1;
                else if (a == 0x69 && b == 0x96)
                    packet = 1;
                else
                    continue;
                
                kfs_patawrite(i, PATA_REG_COMMAND, ATA_CMD_IDENT_PACKET);
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
            kfs_patadrives[count].signature = *(uint16_t *)(read_buffer + ATA_IDENT_SIGNATURE);
            kfs_patadrives[count].capabilities = *(uint16_t *)(read_buffer + ATA_IDENT_CAPABILITIES);
            kfs_patadrives[count].mdma = *(uint16_t *)(read_buffer + 126);
            kfs_patadrives[count].udma = *(uint16_t *)(read_buffer + 176);
            kfs_patadrives[count].commandsets = *(uint32_t *)(read_buffer + ATA_IDENT_COMMANDSETS);

            if (kfs_patadrives[count].commandsets & (1 << 26))
                kfs_patadrives[count].sectors = *(uint32_t *)(read_buffer + ATA_IDENT_MAX_LBA_EXT);
            else
                kfs_patadrives[count].sectors = *(uint32_t *)(read_buffer + ATA_IDENT_MAX_LBA);

            for (int n = 0; n < 40; n += 2)
            {
                kfs_patadrives[count].model[n] = read_buffer[ATA_IDENT_MODEL + n + 1];
                kfs_patadrives[count].model[n + 1] = read_buffer[ATA_IDENT_MODEL + n];
            }
            kfs_patadrives[count].model[40] = 0;
            str_trim(kfs_patadrives[count].model);

            kfs_patawrite(i, PATA_REG_COMMAND, ATA_CMD_IDENT);
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

            /*kdebug_outf("\r\nkfs_i: capabilities %16b", kfs_patadrives[count].capabilities);
            if (!(kfs_patadrives[count].capabilities & (1 << 8)))
                kdebug_outf("\r\nkfs_i: drive not dma capable?");

            kdebug_outf("\r\nkfs_i: mdma %16b", kfs_patadrives[count].mdma);
            kdebug_outf("\r\nkfs_i: udma %16b", kfs_patadrives[count].udma);*/

            kfs_drive *d = kmem_kalloc(sizeof(kfs_drive));
            d->drive_data = &kfs_patadrives[count];
            d->drive_type = KFS_PATA;

            kfs_adddrive(d);

            count++;
        }

        kfs_channel[i].pci = ide_device;
    }

#ifdef AQUA_DEBUG
    uint32_t ints = kpci_configread(ide_device, PCI_OFFSET_HDR0_REGF);
    kdebug_outf("\r\nkfs_i: int pin: %x int line: %x", (ints & 0xFF00) >> 8, ints & 0xFF);
#endif

    uint16_t command = kpci_configread(ide_device, PCI_OFFSET_COMMAND);
    kpci_configwrite16(ide_device, PCI_OFFSET_COMMAND, command | 7);

    kdesc_setinterruptfunc(14, *kfs_patainterrupt1);
    kdesc_setinterruptfunc(15, *kfs_patainterrupt2);
}

int kfs_patadma(kfs_patadrive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr)
{
    kfs_prdt[0] = (1UL << 63) | (sec_count * drive->sector_size << 32) | (uint32_t)((uintptr_t)kfs_patadmabuffer & 0xFFFFFFFF);

    //set direction of data with rw in bm command reg
    outb(kfs_channel[drive->channel].bmide, inb(kfs_channel[drive->channel].bmide) | (read << 3));

    //clear error and interrupt bit in bm status reg
    outb(kfs_channel[drive->channel].bmide + 2, inb(kfs_channel[drive->channel].bmide + 2) | 0x4 | 0x2);

    //send prdt phys addr to bm prdt reg
    outl(kfs_channel[drive->channel].bmide + 4, (uint32_t)((uintptr_t)kfs_prdt) & 0xFFFFFFFF);

    while (kfs_pataread(drive->channel, PATA_REG_STATUS) & ATA_STATUS_BUSY);

    //select drive
    kfs_patawrite(drive->channel, PATA_REG_HDDEVSEL, 0xE0 | (drive->drive << 4));
    ksleep(1);

    //send lba and sec_count to ports
    kfs_patawrite(drive->channel, PATA_REG_SECCNT1, (sec_count >> 8) & 0xFF);
    kfs_patawrite(drive->channel, PATA_REG_LBA3, (lba >> 24) & 0xFF);
    kfs_patawrite(drive->channel, PATA_REG_LBA4, (lba >> 32) & 0xFF);
    kfs_patawrite(drive->channel, PATA_REG_LBA5, (lba >> 40) & 0xFF);

    kfs_patawrite(drive->channel, PATA_REG_SECCNT0, sec_count & 0xFF);
    kfs_patawrite(drive->channel, PATA_REG_LBA0, lba & 0xFF);
    kfs_patawrite(drive->channel, PATA_REG_LBA1, (lba >> 8) & 0xFF);
    kfs_patawrite(drive->channel, PATA_REG_LBA2, (lba >> 16) & 0xFF);

    while (kfs_pataread(drive->channel, PATA_REG_STATUS) & ATA_STATUS_BUSY);
    
    //send dma transfer command to ata controller
    kfs_patawrite(drive->channel, PATA_REG_COMMAND, read ? 0x25 : 0x35);
    ksleep(1);

    //set start/stop bit on bm command register
    outb(kfs_channel[drive->channel].bmide, inb(kfs_channel[drive->channel].bmide) | 1);

    kfs_patawrite(drive->channel, PATA_REG_CONTROL, 0);
    ksleep(1); //give drive time to recieve command

    while (kfs_ataint[drive->channel] == 0)
        asm("hlt");
    kfs_ataint[drive->channel] = 0;

    //read the controller and drive status to check for error
    uint8_t dstatus = kfs_pataread(drive->channel, PATA_REG_STATUS);

    kfs_patawrite(drive->channel, PATA_REG_CONTROL, 2);

    if (dstatus & 0x1)
    {
        kdebug_outf("\r\nkfs_ata: error reading!");
        return -1;
    }

    memcpy_ssealign(addr, kfs_patadmabuffer, drive->sector_size * sec_count);

    return 0;
}