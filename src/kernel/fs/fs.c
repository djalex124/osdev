#include <stdint.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <output/screen.h>

#include <x86_64/pci.h>
#include <x86_64/pit.h>

#include <mm/mem.h>

#include <fs/fs.h>

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

void kfs_printpata(kfs_patadrive *drive)
{
    kscreen_putf("\n PATA drive%d c%d", drive->drive, drive->channel);

    int a = -1, b = 0;
    char* trim = drive->model;
    while (trim[b] != '\0')
    {
        if (trim[b] != ' ')
            a = b;
        b++;
    }
    trim[a + 1] = '\0';

    kscreen_putf(" label [%s]", trim);
}

void kfs_printpartition(kfs_partition *part)
{
    if (part->drive->drive_type == 1)
        kfs_printpata((kfs_patadrive *)part->drive->drive_data);

    if (part->fs == 1)
        kfs_readfat(part);
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

int kfs_readsector(kfs_drive *drive, size_t lba, size_t sec_count, uint8_t read, uint32_t addr)
{
    int result = 0;
    if (drive->drive_type == 1)
        result = kfs_atadma((kfs_patadrive *)drive->drive_data, lba, sec_count, read, addr);
    return result;
}

void kfs_addpartition(kfs_partition* partition)
{
    kfs_partition* ptr = k_infotable.kfs_partitions;
    kdebug_outf("\nkfs_addp: adding");

    if (k_infotable.kfs_partitionsdetected == 0)
        k_infotable.kfs_partitions = partition;
    else
    {
        for (int i = 0; i < k_infotable.kfs_partitionsdetected; i++)
        {
            if ((uintptr_t)ptr->next == 0)
            {
                ptr->next = (uint8_t *)partition;
                break;
            }
            ptr = (kfs_partition *)ptr->next;
        }
    }

    k_infotable.kfs_partitionsdetected++;
}

void kfs_removepartition(kfs_partition* partition)
{
    kfs_partition* ptr = k_infotable.kfs_partitions;
    kdebug_outf("\nkfs_remp: removing");
    for (int i = 0; i < k_infotable.kfs_partitionsdetected; i++)
    {
        if ((uintptr_t)partition == (uintptr_t)ptr->next)
        {
            k_infotable.kfs_partitionsdetected--;
            ptr->next = partition->next;
            break;
        }
        ptr = (kfs_partition *)ptr->next;
    }
    //if this loop ends, partition not found
}

void kfs_init()
{
    size_t i = 0;
    
    for (i = 0; i < k_infotable.kpci_tablesize; i++)
    {
        if (k_infotable.kpci_table[i].class == 0x1 && k_infotable.kpci_table[i].subclass == 0x1)
            kfs_patainit(&k_infotable.kpci_table[i]);
        else if (k_infotable.kpci_table[i].class == 0x1 && k_infotable.kpci_table[i].subclass == 0x6)
            kfs_satainit(&k_infotable.kpci_table[i]);
    }

    for (i = 0; i < 4; i++)
    {
        if (kfs_patadrives[i].exists)
        {
            kfs_drive *patadrive = kfs_patatest(&kfs_patadrives[i]);
            if ((uintptr_t)patadrive)
                kfs_detectfat(patadrive);
        }
    }
}