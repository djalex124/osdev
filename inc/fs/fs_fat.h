#pragma once

#include <fs/fs.h>

void kfs_detectfat(kfs_drive *drive);

uint8_t *kfs_readfilefat(kfs_partition *partition, char *absolutepath, size_t *file_length);
int kfs_checkdirfat(kfs_partition *partition, char *absolutepath);
void kfs_printdirfat(kfs_partition *partition, char *absolutepath);