#ifndef BOOT_H
#define BOOT_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint32_t root_cluster;
} BootSector;

int read_boot_sector(FILE *fp, BootSector *bs);
void print_info(FILE *fp, const BootSector *bs);

#endif