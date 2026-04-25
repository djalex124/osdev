#pragma once

#include <stddef.h>
#include <x86_64/pci.h>

typedef enum {
    KFS_NODRIVE = 0,
    KFS_PATA = 1,
    KFS_SATA = 2,
} kfs_drivetypes;

typedef struct {
    kfs_drivetypes drive_type;
    void *drive_data;
} kfs_drive;

typedef struct {
    uint8_t fs;
    // 1 = fat16
    kfs_drive *drive;
    uint8_t *fs_data;
}__attribute__((packed)) kfs_partition;

extern kfs_drive *kfs_drives[32];

void kfs_printpartition(kfs_partition *part);
void kfs_printinfo();

uint8_t *kfs_readfile(kfs_partition *partition, char *absolutepath, size_t *file_size);
int kfs_checkdir(kfs_partition *partition, char *absolutepath);
void kfs_printdir(kfs_partition *partition, char *absolutepath);

int kfs_readsector(kfs_drive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr);
int kfs_read(kfs_partition *partition, size_t lba, size_t length, uint8_t read, void *addr);

void kfs_addpartition(kfs_partition* partition);
void kfs_removepartition(kfs_partition* partition);

void kfs_adddrive(kfs_drive *drive);
void kfs_removedrive(kfs_drive *drive);

void kfs_init();