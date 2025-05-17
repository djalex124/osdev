#pragma once
#include <pci.h>

void kfs_printinfo();

typedef struct {
    uint8_t exists;
    uint8_t channel;
    uint8_t drive;
    uint16_t type;
    uint16_t signature;
    uint16_t capabilities;
    uint32_t commandsets;
    uint32_t sectors;
    uint32_t sector_size; //bytes
    char model[41];
} kfs_patadrive;

typedef struct {
    uint16_t resv;
    uint16_t byte_count;
    uint32_t prdt_addr;
} kfs_prd;

#ifndef ata_file
extern kfs_patadrive kfs_patadrives[4];
#endif

void kfs_patainit(kpci_device *ide_device);
void kfs_satainit(kpci_device *ide_device);

void kfs_patatest(kfs_patadrive *drive);

void kfs_init();