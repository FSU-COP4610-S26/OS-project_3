#ifndef OPEN_TABLE_H
#define OPEN_TABLE_H

#include <stdint.h>
#include "fat32.h"

#define MAX_OPEN_FILES 10

typedef struct {
    int used;
    char name[20];
    char mode[4];
    char path[1024];
    uint32_t first_cluster;
    uint32_t size;
    uint32_t offset;
} OpenFile;

void init_open_table(void);
int is_valid_mode(const char *mode);
int is_file_open(const char *filename);
int add_open_file(const char *filename, const char *mode,
                  const char *path, uint32_t first_cluster, uint32_t size);
int remove_open_file(const char *filename);
void list_open_files(void);

int set_file_offset(const char *filename, uint32_t new_offset);
int get_file_offset(const char *filename, uint32_t *offset);
int get_file_size_from_open_table(const char *filename, uint32_t *size);
int get_file_mode(const char *filename, char *mode_buffer, uint32_t mode_buffer_size);
int get_file_first_cluster(const char *filename, uint32_t *first_cluster);

int update_file_offset(const char *filename, uint32_t new_offset);
int update_open_file_size(const char *filename, uint32_t new_size);
int update_open_file_first_cluster(const char *filename, uint32_t new_first_cluster);

#endif