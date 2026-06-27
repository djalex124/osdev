#pragma once

#include <fs/fs.h>

#define ATA_PRIMARY   0
#define ATA_SECONDARY 1

#define ATA_BASE_PRIMARY   0x1F0
#define ATA_CTRL_PRIMARY   0x3F6
#define ATA_BASE_SECONDARY 0x170
#define ATA_CTRL_SECONDARY 0x376

#define ATA_STATUS_BUSY  0x80
#define ATA_STATUS_DRQ   0x08
#define ATA_STATUS_ERROR 0x1

#define ATA_IDENT_SIGNATURE    0
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define ATA_CMD_IDENT         0xEC
#define ATA_CMD_IDENT_PACKET  0xA1
#define ATA_CMD_READ_DMA_EXT  0x25
#define ATA_CMD_WRITE_DMA_EXT 0x35

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

    uint32_t sectors;
    uint32_t sector_size; //bytes
    char model[41];
} kfs_satadrive;

int kfs_patadma(kfs_patadrive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr);
int kfs_satadma(kfs_satadrive *drive, size_t lba, size_t sec_count, uint8_t read, void *addr);

void kfs_patainit(kpci_device *ide_device);
void kfs_satainit(kpci_device *ide_device);