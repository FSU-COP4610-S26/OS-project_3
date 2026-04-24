#include <stdio.h>
#include <string.h>
#include "fat32.h"

uint32_t get_image_size(FILE *fp) {
    long current = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, current, SEEK_SET);
    return (uint32_t)size;
}

uint32_t calculate_first_data_sector(const BootSector *bs) {
    return bs->reserved_sector_count + (bs->num_fats * bs->fat_size_32);
}

uint32_t calculate_total_clusters(const BootSector *bs) {
    uint32_t first_data_sector;
    uint32_t data_sectors;

    if (bs->sectors_per_cluster == 0) {
        return 0;
    }

    first_data_sector = calculate_first_data_sector(bs);

    if (bs->total_sectors_32 < first_data_sector) {
        return 0;
    }

    data_sectors = bs->total_sectors_32 - first_data_sector;
    return data_sectors / bs->sectors_per_cluster;
}

int fs_open(FileSystem *fs, const char *image_path) {
    memset(fs, 0, sizeof(FileSystem));

    fs->fp = fopen(image_path, "rb+");
    if (fs->fp == NULL) {
        return -1;
    }

    if (read_boot_sector(fs->fp, &fs->bs) != 0) {
        fclose(fs->fp);
        fs->fp = NULL;
        return -1;
    }

    fs->first_data_sector = calculate_first_data_sector(&fs->bs);
    fs->total_clusters = calculate_total_clusters(&fs->bs);
    fs->cwd_cluster = fs->bs.root_cluster;

    strncpy(fs->image_name, image_path, sizeof(fs->image_name) - 1);
    strncpy(fs->cwd_path, "/", sizeof(fs->cwd_path) - 1);

    return 0;
}

void fs_close(FileSystem *fs) {
    if (fs->fp != NULL) {
        fclose(fs->fp);
        fs->fp = NULL;
    }
}