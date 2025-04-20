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

void kfs_atainit(kpci_device *ide_device);

void kfs_init();