#ifndef DIR_H
#define DIR_H

#include <stdint.h>
#include "fat32.h"

#define ATTR_DIRECTORY 0x10
#define ATTR_LONG_NAME 0x0F

typedef struct __attribute__((packed)) {
    uint8_t name[11];
    uint8_t attr;
    uint8_t ntres;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} DirEntry;

uint32_t cluster_to_offset(const FileSystem *fs, uint32_t cluster);
uint32_t read_fat_entry(const FileSystem *fs, uint32_t cluster);
void write_fat_entry(const FileSystem *fs, uint32_t cluster, uint32_t value);
int is_end_of_chain(uint32_t cluster_value);

void format_name(const uint8_t raw_name[11], char *output);
void make_83_name(const char *input, uint8_t output[11]);

void list_directory(const FileSystem *fs, uint32_t start_cluster);

uint32_t get_entry_cluster(const DirEntry *entry);
int is_directory(const DirEntry *entry);
int find_entry(const FileSystem *fs, uint32_t cluster, const char *name, DirEntry *result);

uint32_t find_free_cluster(const FileSystem *fs);
int find_free_dir_entry(const FileSystem *fs, uint32_t dir_cluster,
                        uint32_t *entry_offset);

int mkdir_in_current_directory(FileSystem *fs, const char *dirname);
int creat_in_current_directory(FileSystem *fs, const char *filename);

int read_file_data(FileSystem *fs, const char *filename, uint32_t size_to_read);
int write_file_data(FileSystem *fs, const char *filename, const char *data);

int remove_file_in_current_directory(FileSystem *fs, const char *filename);
int remove_dir_in_current_directory(FileSystem *fs, const char *dirname);

int move_entry_in_current_directory(FileSystem *fs, const char *source_name,
                                    const char *target_name);

#endif