#pragma once

typedef struct {
    unsigned char reserved;
    unsigned char channel;
    unsigned char drive;
    unsigned short type;
    unsigned short sig;
    unsigned char capabilities;
    unsigned int command_sets;
    unsigned int size;
    unsigned char name[41];
}kfs_idedevice[4];

void kfs_init();