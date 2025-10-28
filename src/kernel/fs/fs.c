#include <stdint.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <kernel/crash.h>

#include <output/kterm.h>

#include <x86_64/pci.h>
#include <x86_64/pit.h>

#include <mm/mem.h>

#include <fs/fs.h>

extern kfs_patadrive kfs_patadrives[4];

void kfs_printreadfile(uint8_t partition, char *filename)
{
    if ((uint64_t)k_infotable.kfs_partitions[partition] == 0)
    {
        kterm_putf("\ninvalid partition number!");
        return;
    }
    if ((uint64_t)filename == 0)
    {
        kterm_putf("\nunable to parse file name!");
        return;
    }
    
    kfs_partition *selected_partition = k_infotable.kfs_partitions[partition];

    size_t file_length = 0;
    uint8_t *file = 0;
    
    if (selected_partition->fs == 1)
        file = kfs_readfilefat(selected_partition, filename, &file_length);

    if ((uint64_t)file)
    {
        kterm_putf("\nhex output 0x%x bytes:\n", file_length);
        int i;
        for (i = 0; i < (file_length - 7); i += 8)
            kterm_putf("%2x%2x%2x%2x%2x%2x%2x%2x", 
                file[i], file[i + 1], file[i + 2], file[i + 3], 
                file[i + 4], file[i + 5], file[i + 6], file[i + 7]);
        for (; i < file_length; i++)
            kterm_putf("%2x", file[i]);
        kmem_free(file, (file_length + 0x1000 - 1) / 0x1000);
    }
    else
        kterm_putf("\nfile not found!");
}

void kfs_printread(uint8_t drive, size_t sector, size_t length)
{
    if (!kfs_patadrives[drive].exists)
    {
        kterm_putf("\ndrive does not exist!");
        return;
    }
    if (sector < 0 || (sector + length) > kfs_patadrives[drive].sectors)
    {
        kterm_putf("\nout of sector bounds! (0 - 0x%x)", kfs_patadrives[drive].sectors);
        return;
    }
    uint8_t *buffer = kmem_alloc(1);
    for (size_t t = sector; t < (sector + length); t++)
    {
        kterm_putf("\nkfs attempting read of drive %d sector %d...", drive, t);
        int error = kfs_atadma(&kfs_patadrives[drive], t, 1, 1, buffer);
        if (error < 0)
        {
            kterm_putf("\nerror during read!");
            kmem_free(buffer, 1);
            return;
        }
        kterm_putf("\nsuccessful read! dumping data to screen...\n");
        size_t not_empty = 0;
        for (int i = 0; i < kfs_patadrives[drive].sector_size; i++)
        {
            if (buffer[i])
            {
                not_empty = 1;
                break;
            }
        }
        if (not_empty)
        {
            for (int i = 0; i < kfs_patadrives[drive].sector_size; i++)
                kterm_putf("%2x", buffer[i]);
        }
        else
        {
            kterm_putf("empty sector");
            ksleep(500);
        }
    }
    kmem_free(buffer, 1);
}

void kfs_printpartition(kfs_partition *part)
{
    if (part->drive->drive_type == 1)
    {
        kfs_patadrive *drive = (kfs_patadrive *)part->drive->drive_data;
        kterm_putf("\n  PATA drive%d c%d label [%s]", drive->drive, drive->channel, drive->model);
    }

    if (part->fs == 1)
        kfs_readfat(part);
}

void kfs_printinfo()
{
    kterm_putf("\nkfs devices currently detected\npata devices:");
    for (int i = 0; i < 4; i++)
    {
        if (kfs_patadrives[i].exists)
        {
            uint64_t size = kfs_patadrives[i].sectors * kfs_patadrives[i].sector_size;
            kterm_putf("\n - device %d [%s] %d MB", i, kfs_patadrives[i].model, size / 1024 / 1024);
        }
    }
}

int kfs_readsector(kfs_drive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr)
{
    int result = 0;
    if (drive->drive_type == 1)
        result = kfs_atadma((kfs_patadrive *)drive->drive_data, lba, sec_count, read, addr);
    return result;
}

int kfs_read(kfs_partition *partition, size_t lba, size_t length, uint8_t read, void *addr)
{
    int result = 0;
    if (partition->drive->drive_type == 1)
    {
        size_t sector_size = ((kfs_patadrive *)partition->drive->drive_data)->sector_size;
        size_t sectors = (length + sector_size - 1) / sector_size;
        kterm_putf("\n%d sectors", sectors);
        for (size_t sector = 0; sector < sectors; sector++)
        {
            result = kfs_readsector(partition->drive, lba + sector, 1, read, addr + (sector_size * sector));

            if (result != 0)
                return result;
        }
    }
    return result;
}

void kfs_addpartition(kfs_partition* partition)
{
    kdebug_outf("\nkfs_addp: adding");

    for (int i = 0; i < 15; i++)
    {
        if (k_infotable.kfs_partitions[i] == 0)
        {
            k_infotable.kfs_partitions[i] = partition;
            return;
        }
    }

    kdebug_outf("\nkfs: failed to add partition!");
}

void kfs_removepartition(kfs_partition* partition)
{
    kdebug_outf("\nkfs_remp: removing");

    for (int i = 0; i < 15; i++)
    {
        kfs_partition* ptr = k_infotable.kfs_partitions[i];
        if ((uintptr_t)partition == (uintptr_t)ptr)
        {
            k_infotable.kfs_partitions[i] = 0;
            return;
        }
    }
    
    kdebug_outf("\nkfs: failed to removed partition!");
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