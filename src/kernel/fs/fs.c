#include <kstring.h>
#include <stdint.h>
#include <kernel.h>
#include <screen.h>
#include <debug.h>
#include <mem.h>
#include <pci.h>
#include <pit.h>
#include <fs.h>

extern kfs_patadrive kfs_patadrives[4];

void kfs_printread(uint8_t drive, size_t sector, size_t length)
{
    if (!kfs_patadrives[drive].exists)
    {
        kscreen_putf("\ndrive does not exist!");
        return;
    }
    if (sector < 0 || (sector + length) > kfs_patadrives[drive].sectors)
    {
        kscreen_putf("\nout of sector bounds! (0 - 0x%x)", kfs_patadrives[drive].sectors);
        return;
    }
    uint8_t *buffer = kmem_alloc(1);
    for (size_t t = sector; t < (sector + length); t++)
    {
        kscreen_putf("\nkfs attempting read of drive %d sector %d...", drive, t);
        int error = kfs_atadma(&kfs_patadrives[drive], t, 1, 1, (uint32_t)((uintptr_t)buffer & 0xFFFFFFFF));
        if (error < 0)
        {
            kscreen_putf("\nerror during read!");
            kmem_free(buffer, 1);
            return;
        }
        kscreen_putf("\nsuccessful read! dumping data to screen...\n");
        size_t not_empty = 0;
        for (int i = 0; i < kfs_patadrives[drive].sector_size; i++)
        {
            if (buffer[i])
                not_empty++;
        }
        if (not_empty)
        {
            for (int i = 0; i < kfs_patadrives[drive].sector_size; i++)
                kscreen_putf("%2x", buffer[i]);
        }
        else
        {
            kscreen_putf("empty sector");
            ksleep(500);
        }
    }
    kmem_free(buffer, 1);
}

void kfs_printinfo()
{
    kscreen_putf("\nkfs devices currently detected\npata devices:");
    for (int i = 0; i < 4; i++)
    {
        if (kfs_patadrives[i].exists)
        {
            uint64_t size = kfs_patadrives[i].sectors * kfs_patadrives[i].sector_size;
            
            int a = -1, b = 0;
            char* trim = kfs_patadrives[i].model;
            while (trim[b] != '\0')
            {
                if (trim[b] != ' ')
                    a = b;
                b++;
            }
            trim[a + 1] = '\0';

            kscreen_putf("\n - device %d [%s] %d MB", i, trim, size / 1024 / 1024);
        }
    }
}

typedef struct
{
    unsigned char 		bootjmp[3];
	unsigned char 		oem_name[8];
	unsigned short 	        bytes_per_sector;
	unsigned char		sectors_per_cluster;
	unsigned short		reserved_sector_count;
	unsigned char		table_count;
	unsigned short		root_entry_count;
	unsigned short		total_sectors_16;
	unsigned char		media_type;
	unsigned short		table_size_16;
	unsigned short		sectors_per_track;
	unsigned short		head_side_count;
	unsigned int 		hidden_sector_count;
	unsigned int 		total_sectors_32;
}__attribute__((packed)) bpb;

typedef struct
{
    uint32_t numsectors;
    uint32_t fatoffset;
    uint32_t numrootenteries;
    uint32_t rootoffset;
    uint32_t rootsize;
    uint32_t fatsize;
    uint32_t fatentrysize;
}fat16_info;

void kfs_readpartitions(kfs_patadrive *drive)
{
    kdebug_outf("\r\nkfs_read: reading MBR from channel %d drive %d",
        drive->channel, drive->drive);
    uint8_t *mbr = kmem_alloc(1);
    kfs_atadma(drive, 0, 1, 1, (uint32_t)((uintptr_t)mbr & 0xFFFFFFFF));

    kdebug_outf("\r\n");
    for (int i = 0x1b8; i < 512; i++)
        kdebug_outf("%2x", mbr[i]);

    size_t entry = 0;
    if (mbr[0x1be] == 0x80)
        entry = 0x1be;
    else if (mbr[0x1ce] == 0x80)
        entry = 0x1ce;
    else if (mbr[0x1de] == 0x80)
        entry = 0x1de;
    else if (mbr[0x1ee] == 0x80)
        entry = 0x1ee;
    
    entry += 8;
    uint32_t start = *(uint32_t*)&mbr[entry];
    kdebug_outf("\r\nstarting lba = %x", start);

    kfs_atadma(drive, start, 2, 1, (uint32_t)((uintptr_t)mbr & 0xFFFFFFFF));
    bpb *esp = (bpb *)mbr;

    fat16_info *info = kmem_kalloc(sizeof(fat16_info));

    kdebug_outf("\r\nmbr strings %8s", (char *)esp->oem_name);
    if (mbr[38] == 0x28 || mbr[38] == 0x29)
    {
        kdebug_outf("\r\nfat12/16");
        uint32_t root_dir_sectors = ((esp->root_entry_count * 32) + (esp->bytes_per_sector - 1)) / esp->bytes_per_sector;
        if (esp->total_sectors_16)
            info->numsectors = esp->total_sectors_16;
        else
            info->numsectors = esp->total_sectors_32;
        info->fatoffset = esp->reserved_sector_count;
        info->fatsize = esp->table_size_16;
        info->fatentrysize = 8;
        info->rootoffset = (esp->table_count * esp->table_size_16) + 1;
        info->rootsize = (esp->root_entry_count * 32) / esp->bytes_per_sector;
        uint32_t first_data_sector = esp->reserved_sector_count + (esp->table_count * esp->table_size_16) + root_dir_sectors;
        uint32_t first_lba = (info->rootoffset * esp->bytes_per_sector) / drive->sector_size;
        kdebug_outf("\r\nfirst data sector lba : %x?", first_lba);
        kfs_atadma(drive, first_lba, 1, 1, (uint32_t)((uintptr_t)mbr & 0xFFFFFFFF));
        kdebug_outf("\r\n");
        for (int i = 0; i < 512; i++)
            kdebug_outf("%2x", mbr[i]);
    }
    else if (mbr[66] == 0x28 || mbr[66] == 0x29)
    {
        kdebug_outf("\r\nfat32");
    }

    kdebug_outf("\r\nfirst fat info:");
    kdebug_outf("\r\n - numsectors %x", info->numsectors);
    kdebug_outf("\r\n - fatsize %x", info->fatsize);
    kdebug_outf("\r\n - rootoffset %x", info->rootoffset);
    kdebug_outf("\r\n - rootsize %x", info->rootsize);

    kmem_free(mbr, 1);
}

void kfs_init()
{
    for (size_t i = 0; i < k_infotable.kpci_tablesize; i++)
    {
        if (k_infotable.kpci_table[i].class == 0x1 && k_infotable.kpci_table[i].subclass == 0x1)
            kfs_patainit(&k_infotable.kpci_table[i]);
        else if (k_infotable.kpci_table[i].class == 0x1 && k_infotable.kpci_table[i].subclass == 0x6)
            kfs_satainit(&k_infotable.kpci_table[i]);
    }

    for (size_t i = 0; i < 4; i++)
    {
        if (kfs_patadrives[i].exists)
        {
            if (kfs_patatest(&kfs_patadrives[i]) == 1)
                kfs_readpartitions(&kfs_patadrives[i]);
        }
    }
}