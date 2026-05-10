#include <stddef.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <output/kterm.h>

#include <x86_64/pci.h>

#include <mm/mem.h>

#include <fs/fs.h>

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

char *kfs_lastentryname = 0;
size_t kfs_lastentrylba = 0;
size_t kfs_lastentrysize = 0;
uint64_t kfs_lastentryattr = 0;

int kfs_readfatentry(kfs_partition *partition, uint8_t *buffer, uint32_t *current_index, uint32_t max_index)
{
    fat16_info *info = (fat16_info *)partition->fs_data;
    
    int continue_loop = 1;

    char *tmp_string = 0;
    size_t tmp_string_len = 0;
    uint32_t index = *current_index;

    if ((uint64_t)kfs_lastentryname)
    {
        kmem_kfree(kfs_lastentryname);
        kfs_lastentryname = 0;
    }

    while (buffer[index] != 0 && index < max_index && continue_loop)
    {
        if (buffer[index] == 0xE5)
        {
            index += 32;
            continue;
        }

        if (buffer[index + 11] == 0x0F)
        {
            formatLFN *lfn = (formatLFN *)&buffer[index];
            if ((uint64_t)tmp_string)
            {
                char* new = kmem_kalloc(13 + tmp_string_len);
                memcpy(new + 13, tmp_string, tmp_string_len);
                kmem_kfree(tmp_string);
                tmp_string = new;
                tmp_string_len += 13;
            }
            else
            {
                tmp_string = kmem_kalloc(14);
                tmp_string_len = 14;
            }
                
            int continuename = 1;
            for (int i = 0; (i < 5) && continuename; i++)
            {
                tmp_string[i] = lfn->name[2 * i];
                if (lfn->name[2 * i] == 0)
                    continuename = 0;
            }
            for (int i = 0; (i < 6) && continuename; i++)
            {
                tmp_string[i + 5] = lfn->name2[2 * i];
                if (lfn->name2[2 * i] == 0)
                    continuename = 0;
            }
            for (int i = 0; (i < 2) && continuename; i++)
            {
                tmp_string[i + 11] = lfn->name3[2 * i];
                if (lfn->name3[2 * i] == 0)
                    continuename = 0;
            }
        }
        else
        {
            format83 *file = (format83 *)&buffer[index];
            uint8_t isfile;
            if (file->attributes & 0x8 || file->attributes & 0x10)
                isfile = 0;
            else
                isfile = 1;

            kfs_lastentryattr = file->attributes;

            if (tmp_string)
            {
                kfs_lastentryname = kmem_kalloc(tmp_string_len);
                memcpy(kfs_lastentryname, tmp_string, tmp_string_len);
                kmem_kfree(tmp_string);
            }
            else
            {
                size_t name_len = 0;
                char *first = str_tok((char *)file->name, " ");
                if (isfile)
                {
                    kfs_lastentryname = kmem_kalloc(13);
                    uint8_t length = str_len(first);
                    if (length > 8)
                        length = 8;
                    memcpy(kfs_lastentryname, first, length);
                    kfs_lastentryname[length] = '.';
                    name_len = length;

                    char *last = str_tok((char *)&file->name[8], " ");

                    length = str_len(last);
                    if (length > 3)
                        length = 3;
                    memcpy(kfs_lastentryname + name_len + 1, last, length);
                }
                else
                {
                    name_len = str_len(first);
                    kfs_lastentryname = kmem_kalloc(name_len + 1);
                    memcpy(kfs_lastentryname, first, name_len);
                }
            }

            if (file->size)
                kfs_lastentrysize = file->size;
            else
                kfs_lastentrysize = 0;

            uint32_t lba = (((file->first_cluster_higher << 16) + file->first_cluster_lower - 2) * info->sectorspercluster)
                + info->rootsize + (info->fatsize * info->fatentrycount) + info->fatoffset + info->startlba;
            kfs_lastentrylba = lba;

            continue_loop = 0;
        }
        index += 32;
    }

    *current_index = index;

    if (index < max_index && buffer[index] != 0)
        return 1;
    return 0;
}

int kfs_readnextcluster(kfs_partition *partition, uint8_t *buffer, uint32_t active_cluster)
{
    fat16_info *info = (fat16_info *)partition->fs_data;

    uint32_t cluster_off = active_cluster * 2;
    kfs_readsector(partition->drive, info->startlba + info->fatoffset + ((cluster_off) / 512), info->sectorspercluster, 1, buffer);
    int table_value = *(unsigned short*)&buffer[cluster_off % 512];

    return table_value;
}

void kfs_readfatlba(kfs_partition *partition, uint32_t selected_lba)
{
    fat16_info *info = (fat16_info *)partition->fs_data;

    uint8_t buffer_size = (info->sectorspercluster * 512 + 0x1000 - 1) / 0x1000;
    uint8_t *buffer = kmem_alloc(buffer_size);

    uint32_t active_cluster = selected_lba / info->sectorspercluster;
    unsigned short table_value = 0;
    while (table_value <= 0xFFF7)
    {
        kfs_readsector(partition->drive, selected_lba, info->sectorspercluster, 1, buffer);
        int cont = 1;

        uint32_t index = 0;
        uint32_t index_max = info->sectorspercluster * 512;
    
        while (cont == 1 && index < index_max)
        {
            kterm_putf("\n");
            cont = kfs_readfatentry(partition, buffer, &index, index_max);
            if (kfs_lastentryattr & 0x8)
                kterm_putf(" VOLUME_ID:");
            else if (kfs_lastentryattr & 0x10)
                kterm_putf(" DIR :");
            else
                kterm_putf(" FILE:");
            kterm_putf(" [%s]", kfs_lastentryname);
            if (kfs_lastentrysize)
                kterm_putf(" - Size 0x%x", kfs_lastentrysize);
        }

        if (index != index_max)
            break;

        active_cluster = kfs_readnextcluster(partition, buffer, active_cluster);
    }

    kmem_free(buffer, buffer_size);
}

int kfs_checkfatlba(kfs_partition *partition, uint32_t selected_lba, const char *name)
{
    fat16_info *info = (fat16_info *)partition->fs_data;

    uint8_t buffer_size = (info->sectorspercluster * 512 + 0x1000 - 1) / 0x1000;
    uint8_t *buffer = kmem_alloc(buffer_size);

    uint32_t active_cluster = selected_lba / info->sectorspercluster;
    unsigned short table_value = 0;

    int entry_found = 0;
    int cont = 1;
    while ((table_value <= 0xFFF7) && (entry_found == 0))
    {
        kfs_readsector(partition->drive, selected_lba, info->sectorspercluster, 1, buffer);

        uint32_t index = 0;
        uint32_t index_max = info->sectorspercluster * 512;
    
        while ((cont == 1) && (index < index_max) && (entry_found == 0))
        {
            cont = kfs_readfatentry(partition, buffer, &index, index_max);
            
            if (((kfs_lastentryattr & 0x10) || (kfs_lastentryattr != 0x08)) && kfs_lastentryname)
            {
                if ((str_len(kfs_lastentryname) == str_len(name)) &&
                    (strn_cmp(name, kfs_lastentryname, str_len(name)) == 0))
                {
                    entry_found = 1;
                    break;
                }
            }
        }

        if (index != index_max || entry_found == 1)
            break;

        active_cluster = kfs_readnextcluster(partition, buffer, active_cluster);
    }

    kmem_free(buffer, buffer_size);

    return entry_found;
}

uint8_t *kfs_readfilefat(kfs_partition *partition, char *absolutepath, size_t *file_length)
{
    fat16_info *info = (fat16_info *)partition->fs_data;
    size_t current_lba = info->startlba + info->fatoffset + (info->fatentrycount * info->fatsize);

    char *tempname = kmem_kalloc(str_len(absolutepath) + 1);
    memcpy(tempname, absolutepath, str_len(absolutepath));
    tempname[str_len(absolutepath)] = 0;
    char *foldername = str_tok(tempname, "/");
    int cont = 1;

    while (cont)
    {
        if (foldername == 0)
        {
            cont = 0;
            continue;
        }

        if (kfs_checkfatlba(partition, current_lba, foldername) == 1)
        {
            if (kfs_lastentryattr & 0x10)
            {
                current_lba = kfs_lastentrylba;
                cont++;
            }
            else if (kfs_lastentryattr != 0x08)
                break;
        }
        else
            cont = 0;

        memcpy(tempname, absolutepath, str_len(absolutepath));
        tempname[str_len(absolutepath)] = 0;
        foldername = str_tok(tempname, "/");
        for (int depth = cont - 1; depth > 0; depth--)
            foldername = str_tok(0, "/");
    }

    kmem_kfree(tempname);

    if (cont >= 1)
    {
        if (kfs_lastentryattr != 0x08 && !(kfs_lastentryattr & 0x10))
        {
            uint8_t *file_data = kmem_alloc((kfs_lastentrysize + 0x1000 - 1) / 0x1000);

            kfs_read(partition, kfs_lastentrylba, kfs_lastentrysize, 1, file_data);
            *file_length = kfs_lastentrysize;

            return file_data;
        }
    }

    return (uint8_t *)0;
}

int kfs_checkdirfat(kfs_partition *partition, char *absolutepath)
{
    if (strn_cmp("/", absolutepath, str_len(absolutepath)) == 0)
    {
        return 1;
    }

    fat16_info *info = (fat16_info *)partition->fs_data;
    size_t current_lba = info->startlba + info->fatoffset + (info->fatentrycount * info->fatsize);

    int count = 0;
    for (int i = 0; i < str_len(absolutepath) - 1; i++)
        if (absolutepath[i] == '/') count++;

    char *tempname = kmem_kalloc(str_len(absolutepath) + 1);
    memcpy(tempname, absolutepath, str_len(absolutepath));
    tempname[str_len(absolutepath)] = 0;
    char *foldername = str_tok(tempname, "/");
    int cont = 1;

    while (cont)
    {
        if (foldername == 0)
        {
            cont = 0;
            continue;
        }

        if (kfs_checkfatlba(partition, current_lba, foldername) == 1)
        {
            if (kfs_lastentryattr & 0x10)
            {
                current_lba = kfs_lastentrylba;
                count--;
                cont++;
            }
            else
                cont = 0;
        }
        else
            cont = 0;

        memcpy(tempname, absolutepath, str_len(absolutepath));
        tempname[str_len(absolutepath)] = 0;
        foldername = str_tok(tempname, "/");
        for (int depth = cont - 1; depth > 0; depth--)
            foldername = str_tok(0, "/");
    }

    kmem_kfree(tempname);

    if (count == 0)
        return 1;

    return 0;
}

void kfs_printdirfat(kfs_partition *partition, char *absolutepath)
{
    fat16_info *info = (fat16_info *)partition->fs_data;
    uint32_t current_lba = info->startlba + info->fatoffset + (info->fatentrycount * info->fatsize);

    if (strn_cmp("/", absolutepath, str_len(absolutepath)) == 0)
    {
        kfs_readfatlba(partition, current_lba);
        return;
    }

    int count = 0;
    for (int i = 0; i < str_len(absolutepath) - 1; i++)
        if (absolutepath[i] == '/') count++;

    char *tempname = kmem_kalloc(str_len(absolutepath) + 1);
    memcpy(tempname, absolutepath, str_len(absolutepath));
    tempname[str_len(absolutepath)] = 0;
    char *foldername = str_tok(tempname, "/");
    int cont = 1;

    while (cont)
    {
        if (foldername == 0)
        {
            cont = 0;
            continue;
        }

        if (kfs_checkfatlba(partition, current_lba, foldername) == 1)
        {
            if (kfs_lastentryattr & 0x10)
            {
                current_lba = kfs_lastentrylba;
                count--;
                cont++;
            }
            else
                cont = 0;
        }
        else
            cont = 0;

        memcpy(tempname, absolutepath, str_len(absolutepath));
        tempname[str_len(absolutepath)] = 0;
        foldername = str_tok(tempname, "/");
        for (int depth = cont - 1; depth > 0; depth--)
            foldername = str_tok(0, "/");
    }

    kmem_kfree(tempname);

    if (count == 0)
        kfs_readfatlba(partition, kfs_lastentrylba);
}

void kfs_detectfat(kfs_drive *drive)
{
    uint8_t *mbr = kmem_alloc(2);

    kfs_readsector(drive, 0, 1, 1, mbr);

    size_t entry = 0;
    if (mbr[0x1be] == 0x80)
        entry = 0x1be;
    else if (mbr[0x1ce] == 0x80)
        entry = 0x1ce;
    else if (mbr[0x1de] == 0x80)
        entry = 0x1de;
    else if (mbr[0x1ee] == 0x80)
        entry = 0x1ee;
    
    uint32_t start = 0;

    if (entry != 0)
    {
        entry += 8;
        start = *(uint32_t*)&mbr[entry];
    }
    else
        kdebug_outf("\nkfs_testfat: no valid mbr, trying zero lba");

    kfs_readsector(drive, start, 2, 1, mbr);
    bpb *esp = (bpb *)mbr;

    //read sector count to discern type of FAT

    kfs_partition* fat_partition = kmem_kalloc(sizeof(kfs_partition));

    uint32_t fat_size = (esp->table_size_16 == 0) ? *(uint32_t *)(mbr + 36) : esp->table_size_16;
    uint32_t total_sectors = (esp->total_sectors_16 == 0) ? esp->total_sectors_32 : esp->total_sectors_16;
    uint32_t root_dir_sectors = ((esp->root_entry_count * 32) + (esp->bytes_per_sector - 1)) / esp->bytes_per_sector;
    uint32_t data_sectors = total_sectors - (esp->reserved_sector_count + (esp->table_count * fat_size) + root_dir_sectors);
    uint32_t total_clusters = data_sectors / esp->sectors_per_cluster;
    kdebug_outf("\nkfs_testfat: total clusters %d", total_clusters);

    //kdebug_outf("\r\nmbr strings %8s", (char *)esp->oem_name);
    if (total_clusters > 0 && total_clusters < 65525)
    {
        fat16_info *info = kmem_kalloc(sizeof(fat16_info));
        kdebug_outf("\r\nkfs_testfat: fat12/16 detected");
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
        kmem_kfree(fat_partition);
        kmem_free(mbr, 2);
        return;
    }

    kmem_free(mbr, 2);

    kfs_addpartition(fat_partition);
}