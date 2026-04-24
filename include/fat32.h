#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>
#include "boot.h"

typedef struct {
    FILE *fp;
    BootSector bs;
    uint32_t first_data_sector;
    uint32_t total_clusters;
    char image_name[256];
    char cwd_path[1024];
    uint32_t cwd_cluster;
} FileSystem;

int fs_open(FileSystem *fs, const char *image_path);
void fs_close(FileSystem *fs);
uint32_t get_image_size(FILE *fp);
uint32_t calculate_first_data_sector(const BootSector *bs);
uint32_t calculate_total_clusters(const BootSector *bs);

#endif