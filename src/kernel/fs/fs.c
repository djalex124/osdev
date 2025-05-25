#include <kstring.h>
#include <stdint.h>
#include <kernel.h>
#include <screen.h>
#include <debug.h>
#include <mem.h>
#include <pci.h>
#include <fs.h>

extern kfs_patadrive kfs_patadrives[4];

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
    uint8_t jump[3];
    uint8_t oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_FAT;
    uint16_t num_root;
    uint16_t total_sectors;
    uint8_t media_type;
    uint16_t resv;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t num_hidden_sectors;
    uint32_t large_sectors;
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backupboot_sector;
    uint8_t resv2[12];
    uint8_t drive_number;
    uint8_t windows_flags;
    uint8_t signature; //0x28 or 0x29
    uint32_t volume_idnumber;
    uint8_t volume_label[11];
    uint8_t systemid_label[8];
}fat32;

void kfs_readpartitions(kfs_patadrive *drive)
{
    kdebug_outf("\r\nkfs_read: reading MBR from channel %d drive %d",
        drive->channel, drive->drive);
    uint8_t *mbr = kmem_alloc(1);
    kfs_atadma(drive, 0, 1, 1, (uint32_t)((uintptr_t)mbr & 0xFFFFFFFF));

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

    kfs_atadma(drive, start, 1, 1, (uint32_t)((uintptr_t)mbr & 0xFFFFFFFF));
    fat32 *esp = (fat32 *)mbr;

    kdebug_outf("\r\nmbr strings %8s", (char *)esp->oem_id);

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