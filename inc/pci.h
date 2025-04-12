#pragma once

#include <stdint.h>

void kpci_init();
void kpci_printinfo();

typedef struct
{
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    
    uint16_t vendorid;
    uint16_t deviceid;
    uint16_t command;
    uint16_t status;
    uint8_t revid;
    uint8_t progif;
    uint8_t subclass;
    uint8_t class;
    uint8_t cls;
    uint8_t lt;
    uint8_t headertype;
    uint8_t bist;
}kpci_headercommon;