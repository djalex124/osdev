#pragma once
#include <pci.h>

void kfs_printread(uint8_t drive, size_t sector, size_t sector_end);
void kfs_printinfo();

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

#ifndef ata_file
extern kfs_patadrive kfs_patadrives[4];
#endif

int kfs_atadma(kfs_patadrive *drive, size_t lba, size_t sec_count, uint8_t read, uint32_t addr);

uint8_t kfs_patatest(kfs_patadrive *drive);

void kfs_patainit(kpci_device *ide_device);
void kfs_satainit(kpci_device *ide_device);

void kfs_init();