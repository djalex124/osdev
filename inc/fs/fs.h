#pragma once
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

typedef struct {
    uint8_t drive_type;
    // 1 = pata
    // 2 = sata
    uint8_t *drive_data;
} kfs_drive;

typedef struct {
    uint8_t fs;
    // 1 = fat16
    kfs_drive *drive;
    uint8_t *fs_data;
}__attribute__((packed)) kfs_partition;

#ifndef ata_file
extern kfs_patadrive kfs_patadrives[4];
#endif

int kfs_atadma(kfs_patadrive *drive, size_t lba, size_t sec_count, uint8_t read, uint32_t addr);

int kfs_readsector(kfs_drive *drive, size_t lba, size_t sec_count, uint8_t read, uint32_t addr);
int kfs_read(kfs_partition *partition, size_t lba, size_t length, uint8_t read, uint32_t addr);

void kfs_detectfat(kfs_drive *drive);
void kfs_readfat(kfs_partition *partition);
uint8_t *kfs_readfilefat(kfs_partition *partition, char *filename, size_t *file_length);

kfs_drive* kfs_patatest(kfs_patadrive *drive);

void kfs_patainit(kpci_device *ide_device);
void kfs_satainit(kpci_device *ide_device);

void kfs_printpartition(kfs_partition *part);
void kfs_printreadfile(uint8_t drive, char *filename);
void kfs_printread(uint8_t drive, size_t sector, size_t sector_end);
void kfs_printinfo();

void kfs_addpartition(kfs_partition* partition);
void kfs_removepartition(kfs_partition* partition);

void kfs_init();