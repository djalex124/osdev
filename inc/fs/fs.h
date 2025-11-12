#pragma once

#include <stddef.h>
#include <x86_64/pci.h>

typedef struct {
    uint8_t exists;
    uint8_t channel;
    uint8_t drive;
    uint16_t type;
    uint16_t signature;
    uint16_t capabilities;
    uint16_t mdma;
    uint16_t udma;
    uint32_t commandsets;
    uint32_t sectors;
    uint32_t sector_size; //bytes
    char model[41];
} kfs_patadrive;

typedef enum {
    AHCI_ATA = 0,
    AHCI_ATAPI = 1,
    AHCI_PM = 2,
    AHCI_SEMB = 3,
} kfs_satatypes;

typedef struct {
    uint32_t drive;
    kfs_satatypes type;
    kpci_device *ahci_controller;
} kfs_satadrive;

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

int kfs_patadma(kfs_patadrive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr);

int kfs_readsector(kfs_drive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr);
int kfs_read(kfs_partition *partition, size_t lba, size_t length, uint8_t read, void *addr);

void kfs_detectfat(kfs_drive *drive);
void kfs_readfat(kfs_partition *partition);
uint8_t *kfs_readfilefat(kfs_partition *partition, char *filename, size_t *file_length);

int kfs_patatest(kfs_patadrive *drive);

void kfs_patainit(kpci_device *ide_device);
void kfs_satainit(kpci_device *ide_device);

void kfs_printpartition(kfs_partition *part);
void kfs_printreadfile(uint8_t drive, char *filename);
void kfs_printinfo();

void kfs_addpartition(kfs_partition* partition);
void kfs_removepartition(kfs_partition* partition);

void kfs_adddrive(kfs_drive *drive);
void kfs_removedrive(kfs_drive *drive);

void kfs_init();