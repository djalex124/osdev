#include <stddef.h>
#include <screen.h>
#include <kstring.h>
#include <debug.h>
#include <mem.h>
#include <pci.h>
#include <fs.h>

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
    uint32_t fatentrycount;
    uint32_t startlba;
    uint8_t sectorspercluster;
}fat16_info;

typedef struct
{
    uint8_t name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_hundredths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t accessed_date;
    uint16_t first_cluster_higher;
    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t first_cluster_lower;
    uint32_t size;
}format83;

typedef struct
{
    uint8_t order;
    uint8_t name[10];
    uint8_t attributes;
    uint8_t entry_type;
    uint8_t checksum;
    uint8_t name2[12];
    uint16_t zero;
    uint8_t name3[4];
}formatLFN;

void kfs_readfat(kfs_partition *partition)
{
    uint8_t *buffer = kmem_alloc(1);

    fat16_info *info = (fat16_info *)partition->fs_data;
    
    uint32_t lba_root_dir = info->startlba + info->fatoffset + (info->fatentrycount * info->fatsize);
    //kdebug_outf("\r\n - lba_root_dir = 0x%x", lba_root_dir);
    //kdebug_outf("\r\n - sectors_per_cluster = 0x%x", info->sectorspercluster);
    kscreen_putf("\nFAT16:");

    kfs_readsector(partition->drive, lba_root_dir, 1, 1, (uint32_t)((uintptr_t)buffer & 0xFFFFFFFF));

    uint32_t index = 0;
    char* tmp_string = 0;
    while (buffer[index] && index < 512)
    {
        if (buffer[index] == 0xE5)
        {
            index += 32;
            continue;
        }
        
        if (buffer[index + 11] == 0x0F)
        {
            formatLFN *lfn = (formatLFN *)&buffer[index];
            //kscreen_putf("\n LFN entry -");
            //kscreen_putf(" index %x ", lfn->order);
            if (tmp_string)
            {
                kmem_kalloc(13);
                memcpy(tmp_string + 13, tmp_string, 13);
            }
            else
                tmp_string = kmem_kalloc(13);
            for (int i = 0; i < 5; i++)
                tmp_string[i] = lfn->name[2 * i];
            for (int i = 0; i < 6; i++)
                tmp_string[i + 5] = lfn->name2[2 * i];
            for (int i = 0; i < 2; i++)
                tmp_string[i + 11] = lfn->name3[2 * i];
        }
        else
        {
            format83 *file = (format83 *)&buffer[index];
            kscreen_putf("\n");
            if (file->attributes & 0x8)
                kscreen_putf(" VOLUME_ID:");
            else if (file->attributes & 0x10)
                kscreen_putf(" DIRECTORY:");
            else
                kscreen_putf(" FILE:");
            if (tmp_string)
            {
                kscreen_putf(" LFN %s", tmp_string);
                kmem_kfree(((str_len(tmp_string) + 12) / 13) * 13);
                tmp_string = 0;
            }
            else
                kscreen_putf(" %11s", file->name);
            if (file->size)
                kscreen_putf(" SIZE: 0x%x bytes", file->size);
            uint32_t lba = (((file->first_cluster_higher << 16) + file->first_cluster_lower - 2) * info->sectorspercluster)
                + info->rootsize + lba_root_dir;
            kscreen_putf(" LBA: 0x%x", lba);
        }
        index += 32;
    }

    kmem_free(buffer, 1);
}

kfs_partition* kfs_detectfat(kfs_drive *drive)
{
    uint8_t *mbr = kmem_alloc(1);

    kfs_readsector(drive, 0, 1, 1, (uint32_t)((uintptr_t)mbr & 0xFFFFFFFF));

    //kdebug_outf("\r\n");
    //for (int i = 0x1b8; i < 512; i++)
    //    kdebug_outf("%2x", mbr[i]);

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
    //kdebug_outf("\r\nstarting lba = %x", start);

    kfs_readsector(drive, start, 2, 1, (uint32_t)((uintptr_t)mbr & 0xFFFFFFFF));
    bpb *esp = (bpb *)mbr;

    //read sector count to discern type of FAT

    kfs_partition* fat_partition = kmem_kalloc(sizeof(kfs_partition));

    //kdebug_outf("\r\nmbr strings %8s", (char *)esp->oem_name);
    if (mbr[38] == 0x28 || mbr[38] == 0x29)
    {
        fat16_info *info = kmem_kalloc(sizeof(fat16_info));
        kdebug_outf("\r\nkfs_test: fat12/16 detected");
        if (esp->total_sectors_16)
            info->numsectors = esp->total_sectors_16;
        else
            info->numsectors = esp->total_sectors_32;
        info->fatoffset = esp->reserved_sector_count;
        info->fatsize = esp->table_size_16;
        info->fatentrycount = esp->table_count;
        info->rootoffset = (esp->table_count * esp->table_size_16) + 1;
        info->rootsize = ((esp->root_entry_count * 32) + esp->bytes_per_sector - 1) / esp->bytes_per_sector;
        info->startlba = start;
        info->sectorspercluster = esp->sectors_per_cluster;

        fat_partition->fs = 1;
        fat_partition->fs_data = (uint8_t *)info;
        fat_partition->drive = drive;
    }
    //else if (mbr[66] == 0x28 || mbr[66] == 0x29)
    //{
    //    kdebug_outf("\r\nfat32");
    //}
    else
    {
        kmem_kfree(sizeof(kfs_partition));
        kmem_free(mbr, 1);
        return NULL;
    }

    kmem_free(mbr, 1);

    return fat_partition;
}