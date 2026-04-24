#include <stdio.h>
#include <stdint.h>
#include "boot.h"

int read_boot_sector(FILE *fp, BootSector *bs) {
    uint16_t signature;

    if (fp == NULL || bs == NULL) {
        return -1;
    }

    fseek(fp, 11, SEEK_SET);
    if (fread(&bs->bytes_per_sector, sizeof(uint16_t), 1, fp) != 1) return -1;

    fseek(fp, 13, SEEK_SET);
    if (fread(&bs->sectors_per_cluster, sizeof(uint8_t), 1, fp) != 1) return -1;

    fseek(fp, 14, SEEK_SET);
    if (fread(&bs->reserved_sector_count, sizeof(uint16_t), 1, fp) != 1) return -1;

    fseek(fp, 16, SEEK_SET);
    if (fread(&bs->num_fats, sizeof(uint8_t), 1, fp) != 1) return -1;

    fseek(fp, 32, SEEK_SET);
    if (fread(&bs->total_sectors_32, sizeof(uint32_t), 1, fp) != 1) return -1;

    fseek(fp, 36, SEEK_SET);
    if (fread(&bs->fat_size_32, sizeof(uint32_t), 1, fp) != 1) return -1;

    fseek(fp, 44, SEEK_SET);
    if (fread(&bs->root_cluster, sizeof(uint32_t), 1, fp) != 1) return -1;

    fseek(fp, 510, SEEK_SET);
    if (fread(&signature, sizeof(uint16_t), 1, fp) != 1) return -1;

    if (signature != 0xAA55) return -1;
    if (bs->bytes_per_sector == 0) return -1;
    if (bs->sectors_per_cluster == 0) return -1;
    if (bs->num_fats == 0) return -1;
    if (bs->fat_size_32 == 0) return -1;
    if (bs->total_sectors_32 == 0) return -1;
    if (bs->root_cluster < 2) return -1;

    return 0;
}

void print_info(FILE *fp, const BootSector *bs) {
    uint32_t first_data_sector;
    uint32_t data_sectors;
    uint32_t total_clusters;
    uint32_t fat_entries;
    long current;
    long image_size;

    if (fp == NULL || bs == NULL) {
        return;
    }

    first_data_sector = bs->reserved_sector_count + (bs->num_fats * bs->fat_size_32);
    data_sectors = bs->total_sectors_32 - first_data_sector;
    total_clusters = data_sectors / bs->sectors_per_cluster;
    fat_entries = (bs->fat_size_32 * bs->bytes_per_sector) / 4;

    current = ftell(fp);
    fseek(fp, 0, SEEK_END);
    image_size = ftell(fp);
    fseek(fp, current, SEEK_SET);

    printf("position of root cluster (in cluster #): %u\n", bs->root_cluster);
    printf("bytes per sector: %u\n", bs->bytes_per_sector);
    printf("sectors per cluster: %u\n", bs->sectors_per_cluster);
    printf("total # of clusters in data region: %u\n", total_clusters);
    printf("# of entries in one FAT: %u\n", fat_entries);
    printf("size of image (in bytes): %ld\n", image_size);
}