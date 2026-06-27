#pragma once

#include <fs/fs.h>

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
    void *port;
} kfs_satadrive;

int kfs_patadma(kfs_patadrive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr);

int kfs_pata_mbrtest(kfs_patadrive *drive);
int kfs_sata_mbrtest(kfs_satadrive *drive);

void kfs_patainit(kpci_device *ide_device);
void kfs_satainit(kpci_device *ide_device);