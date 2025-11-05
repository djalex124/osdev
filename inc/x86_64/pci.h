#pragma once

#include <stdint.h>

#define AQUA_IDE_DEBUG

#define PCI_OFFSET_VENDORID   0x0
#define PCI_OFFSET_DEVICEID   0x2
#define PCI_OFFSET_COMMAND    0x4
#define PCI_OFFSET_STATUS     0x6
#define PCI_OFFSET_REVISIONID 0x8
#define PCI_OFFSET_PROGIF     0x9
#define PCI_OFFSET_SUBCLASS   0xA
#define PCI_OFFSET_CLASS      0xB
#define PCI_OFFSET_CACHELNSZ  0xC
#define PCI_OFFSET_LATTIMER   0xD
#define PCI_OFFSET_HDRTYPE    0xE
#define PCI_OFFSET_BIST       0xF

#define PCI_OFFSET_HDR0_BAR0  0x10
#define PCI_OFFSET_HDR0_BAR1  0x14
#define PCI_OFFSET_HDR0_BAR2  0x18
#define PCI_OFFSET_HDR0_BAR3  0x1C
#define PCI_OFFSET_HDR0_BAR4  0x20
#define PCI_OFFSET_HDR0_BAR5  0x24
#define PCI_OFFSET_HDR0_REGF  0x3C

char* kpci_getsubclassname(uint8_t class, uint8_t subclass);
char* kpci_getclassname(uint8_t class);

typedef struct
{
    uint16_t section;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
}kpci_device;

uint16_t kpci_getdeviceid(kpci_device *device);
uint16_t kpci_getvendorid(kpci_device *device);
uint8_t kpci_getbaseclass(kpci_device *device);
uint8_t kpci_getsubclass(kpci_device *device);

extern uint32_t (*kpci_configread)(kpci_device *device, uint8_t off);
extern void (*kpci_configwrite16)(kpci_device* device, uint8_t off, uint16_t val);
void kpci_init();