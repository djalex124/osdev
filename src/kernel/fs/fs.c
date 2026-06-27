#include <stdint.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <kernel/crash.h>

#include <output/kterm.h>

#include <x86_64/pci.h>
#include <x86_64/pit.h>

#include <mm/mem.h>

#include <fs/fs_ata.h>
#include <fs/fs_fat.h>
#include <fs/fs.h>

kfs_drive *kfs_drives[32];

uint8_t *kfs_readfile(kfs_partition *partition, char *absolutepath, size_t *file_size)
{
    uint8_t *file = 0;
    if (partition->fs == 1)
        file = kfs_readfilefat(partition, absolutepath, file_size);

    return file;
}

int kfs_checkdir(kfs_partition *partition, char *absolutepath)
{
    int exists = 0;
    if (partition->fs == 1)
        exists = kfs_checkdirfat(partition, absolutepath);

    return exists;
}

void kfs_printdir(kfs_partition *partition, char *absolutepath)
{
    if (partition->fs == 1)
        kfs_printdirfat(partition, absolutepath);
}

void kfs_printpartition(kfs_partition *part)
{
    if (part->drive->drive_type == 1)
    {
        kfs_patadrive *drive = (kfs_patadrive *)part->drive->drive_data;
        kterm_putf(" PATA drive%d c%d label [%s]", drive->drive, drive->channel, drive->model);
    }
}

void kfs_printinfo()
{
    kterm_putf("\nkfs drives detected:");
    int drives = 0;

    for (int i = 0; i < 32; i++)
    {
        if (kfs_drives[i])
        {
            kfs_drive *drive = kfs_drives[i];
            if (drive->drive_type == KFS_PATA)
            {
                kfs_patadrive *patadrive = (kfs_patadrive *)drive->drive_data;
                uint64_t size = patadrive->sectors * patadrive->sector_size;
                kterm_putf("\n %2d - PATA device [%s]", i, patadrive->model);
                kterm_putf("\n    - %d MB", size / 1024 / 1024);
            }
            else if (drive->drive_type == KFS_SATA)
            {
                kfs_satadrive *satadrive = (kfs_satadrive *)drive->drive_data;
                kterm_putf("\n %2d - SATA device (Slot %d) ", i, satadrive->drive);
                if (satadrive->type == AHCI_ATA)
                    kterm_putf("ATA");
                else if (satadrive->type == AHCI_ATAPI)
                    kterm_putf("ATAPI");
                else
                    kterm_putf("Other(%d)", satadrive->type);
            }
            else
                kterm_putf("\n %d - Drive detected, unknown", i);
            drives = 1;
        }
    }

    if (drives == 0)
        kterm_putf("\n - No drives currently detected");
}

int kfs_readsector(kfs_drive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr)
{
    int result = 0;
    if (drive->drive_type == KFS_PATA)
        result = kfs_patadma((kfs_patadrive *)drive->drive_data, lba, sec_count, read, addr);
    else if (drive->drive_type == KFS_SATA)
        kdebug_outf("\nkfs_readsector: sata not impl yet");
    return result;
}

int kfs_read(kfs_partition *partition, size_t lba, size_t length, uint8_t read, void *addr)
{
    int result = 0;
    if (partition->drive->drive_type == KFS_PATA)
    {
        size_t sector_size = ((kfs_patadrive *)partition->drive->drive_data)->sector_size;
        size_t sectors = (length + sector_size - 1) / sector_size;
        kterm_putf("\n%d sectors", sectors);
        size_t sector;
        for (sector = 0; sectors - sector > 64; sector += 64)
        {
            result = kfs_readsector(partition->drive, lba + sector, 64, read, addr + (sector_size * sector));

            if (result != 0)
                return result;
        }
        if (sectors - sector)
        {
            result = kfs_readsector(partition->drive, lba + sector, sectors - sector, read, addr + (sector_size * sector));

            if (result != 0)
                return result;
        }
    }
    return result;
}

void kfs_addpartition(kfs_partition* partition)
{
    kdebug_outf("\nkfs_addp: adding");

    for (int i = 0; i < 16; i++)
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

    for (int i = 0; i < 16; i++)
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

void kfs_adddrive(kfs_drive *drive)
{
    int i = 0;
    for (; i < 32; i++)
    {
        if (kfs_drives[i] == 0)
        {
            kfs_drives[i] = drive;
            break;
        }
    }

    if (i == 32)
        kdebug_outf("\nkfs: too many drives! not adding...");
}

void kfs_removedrive(kfs_drive *drive)
{
    int i = 0;
    for (; i < 32; i++)
    {
        if (kfs_drives[i] == drive)
        {
            kfs_drives[i] = 0;
            break;
        }
    }

    if (i == 32)
        kdebug_outf("\nkfs: attempting to remove unmapped drive!");
}

void kfs_init()
{
    size_t i = 0;
    
    for (i = 0; i < k_infotable.kpci_tablesize; i++)
    {
        if (kpci_getbaseclass(&k_infotable.kpci_table[i]) == 0x1 && kpci_getsubclass(&k_infotable.kpci_table[i]) == 0x1)
            kfs_patainit(&k_infotable.kpci_table[i]);
        else if (kpci_getbaseclass(&k_infotable.kpci_table[i]) && kpci_getsubclass(&k_infotable.kpci_table[i]) == 0x6)
            kfs_satainit(&k_infotable.kpci_table[i]);
    }

    for (i = 0; i < 32; i++)
    {
        if (kfs_drives[i])
        {
            kfs_drive *drive = kfs_drives[i];
            int working = 0;
            switch (drive->drive_type)
            {
                case KFS_PATA:
                    working = kfs_pata_mbrtest((kfs_patadrive *)drive->drive_data);
                    break;
                case KFS_SATA:
                    working = kfs_sata_mbrtest((kfs_satadrive *)drive->drive_data);
                    break;
                default:
                    break;
            }

            if (working > 0)
                kfs_detectfat(drive);
        }
    }
}