#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "dir.h"
#include "open_table.h"

uint32_t cluster_to_offset(const FileSystem *fs, uint32_t cluster) {
    uint32_t first_sector_of_cluster;

    first_sector_of_cluster =
        ((cluster - 2) * fs->bs.sectors_per_cluster) + fs->first_data_sector;

    return first_sector_of_cluster * fs->bs.bytes_per_sector;
}

uint32_t read_fat_entry(const FileSystem *fs, uint32_t cluster) {
    uint32_t fat_offset;
    uint32_t fat_byte_offset;
    uint32_t value;

    fat_offset = cluster * 4;
    fat_byte_offset =
        (fs->bs.reserved_sector_count * fs->bs.bytes_per_sector) + fat_offset;

    fseek(fs->fp, fat_byte_offset, SEEK_SET);
    fread(&value, sizeof(uint32_t), 1, fs->fp);

    return value & 0x0FFFFFFF;
}

void write_fat_entry(const FileSystem *fs, uint32_t cluster, uint32_t value) {
    uint32_t fat_offset;
    uint32_t fat_byte_offset;
    uint32_t i;
    uint32_t stored_value;

    fat_offset = cluster * 4;
    stored_value = value & 0x0FFFFFFF;

    for (i = 0; i < fs->bs.num_fats; i++) {
        fat_byte_offset =
            ((fs->bs.reserved_sector_count + (i * fs->bs.fat_size_32))
             * fs->bs.bytes_per_sector) + fat_offset;

        fseek(fs->fp, fat_byte_offset, SEEK_SET);
        fwrite(&stored_value, sizeof(uint32_t), 1, fs->fp);
    }

    fflush(fs->fp);
}

int is_end_of_chain(uint32_t cluster_value) {
    return cluster_value >= 0x0FFFFFF8;
}

void format_name(const uint8_t raw_name[11], char *output) {
    char name[9];
    char ext[4];
    int i;
    int name_len = 0;
    int ext_len = 0;

    memset(name, 0, sizeof(name));
    memset(ext, 0, sizeof(ext));

    if (raw_name[0] == '.' && raw_name[1] == ' ') {
        snprintf(output, 20, ".");
        return;
    }

    if (raw_name[0] == '.' && raw_name[1] == '.' && raw_name[2] == ' ') {
        snprintf(output, 20, "..");
        return;
    }

    for (i = 0; i < 8 && raw_name[i] != ' '; i++) {
        name[name_len++] = (char)raw_name[i];
    }

    for (i = 8; i < 11 && raw_name[i] != ' '; i++) {
        ext[ext_len++] = (char)raw_name[i];
    }

    if (ext_len > 0) {
        snprintf(output, 20, "%s.%s", name, ext);
    } else {
        snprintf(output, 20, "%s", name);
    }
}

void make_83_name(const char *input, uint8_t output[11]) {
    size_t i;

    memset(output, ' ', 11);

    if (strcmp(input, ".") == 0) {
        output[0] = '.';
        return;
    }

    if (strcmp(input, "..") == 0) {
        output[0] = '.';
        output[1] = '.';
        return;
    }

    for (i = 0; i < 11 && input[i] != '\0'; i++) {
        char c = input[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        output[i] = (uint8_t)c;
    }
}

uint32_t get_entry_cluster(const DirEntry *entry) {
    return ((uint32_t)entry->first_cluster_high << 16) |
           entry->first_cluster_low;
}

int is_directory(const DirEntry *entry) {
    return (entry->attr & ATTR_DIRECTORY) != 0;
}

int find_entry(const FileSystem *fs, uint32_t cluster, const char *name, DirEntry *result) {
    uint32_t current_cluster = cluster;
    uint32_t offset;
    uint32_t entries_per_cluster;
    uint32_t i;
    DirEntry entry;
    char formatted_name[20];
    char target_name[20];
    size_t j;

    memset(target_name, 0, sizeof(target_name));
    for (j = 0; j < sizeof(target_name) - 1 && name[j] != '\0'; j++) {
        char c = name[j];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        target_name[j] = c;
    }

    entries_per_cluster =
        (fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster) / sizeof(DirEntry);

    while (!is_end_of_chain(current_cluster)) {
        offset = cluster_to_offset(fs, current_cluster);

        for (i = 0; i < entries_per_cluster; i++) {
            fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
            fread(&entry, sizeof(DirEntry), 1, fs->fp);

            if (entry.name[0] == 0x00) {
                return -1;
            }

            if (entry.name[0] == 0xE5) {
                continue;
            }

            if (entry.attr == ATTR_LONG_NAME) {
                continue;
            }

            format_name(entry.name, formatted_name);

            if (strcmp(formatted_name, target_name) == 0 ||
                strcmp(formatted_name, name) == 0) {
                *result = entry;
                return 0;
            }
        }

        current_cluster = read_fat_entry(fs, current_cluster);
    }

    return -1;
}

void list_directory(const FileSystem *fs, uint32_t start_cluster) {
    uint32_t current_cluster = start_cluster;
    uint32_t offset;
    uint32_t entries_per_cluster;
    uint32_t i;
    DirEntry entry;
    char formatted_name[20];

    if (start_cluster < 2) {
        printf("Error: invalid directory cluster\n");
        return;
    }

    entries_per_cluster =
        (fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster) / sizeof(DirEntry);

    while (!is_end_of_chain(current_cluster)) {
        offset = cluster_to_offset(fs, current_cluster);

        for (i = 0; i < entries_per_cluster; i++) {
            fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
            fread(&entry, sizeof(DirEntry), 1, fs->fp);

            if (entry.name[0] == 0x00) {
                return;
            }

            if (entry.name[0] == 0xE5) {
                continue;
            }

            if (entry.attr == ATTR_LONG_NAME) {
                continue;
            }

            format_name(entry.name, formatted_name);
            printf("%s\n", formatted_name);
        }

        current_cluster = read_fat_entry(fs, current_cluster);
    }
}

uint32_t find_free_cluster(const FileSystem *fs) {
    uint32_t cluster;

    for (cluster = 2; cluster < fs->total_clusters + 2; cluster++) {
        if (read_fat_entry(fs, cluster) == 0) {
            return cluster;
        }
    }

    return 0;
}

int find_free_dir_entry(const FileSystem *fs, uint32_t dir_cluster,
                        uint32_t *entry_offset) {
    uint32_t current_cluster = dir_cluster;
    uint32_t offset;
    uint32_t entries_per_cluster;
    uint32_t i;
    DirEntry entry;

    entries_per_cluster =
        (fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster) / sizeof(DirEntry);

    while (!is_end_of_chain(current_cluster)) {
        offset = cluster_to_offset(fs, current_cluster);

        for (i = 0; i < entries_per_cluster; i++) {
            fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
            fread(&entry, sizeof(DirEntry), 1, fs->fp);

            if (entry.name[0] == 0x00 || entry.name[0] == 0xE5) {
                *entry_offset = offset + (i * sizeof(DirEntry));
                return 0;
            }
        }

        current_cluster = read_fat_entry(fs, current_cluster);
    }

    return -1;
}

static void write_empty_cluster(const FileSystem *fs, uint32_t cluster) {
    uint32_t offset;
    uint32_t cluster_size;
    uint8_t zero = 0;
    uint32_t i;

    offset = cluster_to_offset(fs, cluster);
    cluster_size = fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster;

    fseek(fs->fp, offset, SEEK_SET);
    for (i = 0; i < cluster_size; i++) {
        fwrite(&zero, 1, 1, fs->fp);
    }
    fflush(fs->fp);
}

static void init_dot_entries(const FileSystem *fs, uint32_t new_cluster,
                             uint32_t parent_cluster) {
    DirEntry dot;
    DirEntry dotdot;
    uint32_t offset;

    memset(&dot, 0, sizeof(DirEntry));
    memset(&dotdot, 0, sizeof(DirEntry));

    make_83_name(".", dot.name);
    dot.attr = ATTR_DIRECTORY;
    dot.first_cluster_high = (uint16_t)(new_cluster >> 16);
    dot.first_cluster_low = (uint16_t)(new_cluster & 0xFFFF);

    make_83_name("..", dotdot.name);
    dotdot.attr = ATTR_DIRECTORY;
    dotdot.first_cluster_high = (uint16_t)(parent_cluster >> 16);
    dotdot.first_cluster_low = (uint16_t)(parent_cluster & 0xFFFF);

    offset = cluster_to_offset(fs, new_cluster);

    fseek(fs->fp, offset, SEEK_SET);
    fwrite(&dot, sizeof(DirEntry), 1, fs->fp);
    fwrite(&dotdot, sizeof(DirEntry), 1, fs->fp);
    fflush(fs->fp);
}

static int update_dir_entry_for_file(FileSystem *fs, const char *filename,
                                     uint32_t first_cluster, uint32_t file_size) {
    uint32_t current_cluster;
    uint32_t offset;
    uint32_t entries_per_cluster;
    uint32_t i;
    DirEntry entry;
    char formatted_name[20];
    char target_name[20];
    size_t j;

    memset(target_name, 0, sizeof(target_name));
    for (j = 0; j < sizeof(target_name) - 1 && filename[j] != '\0'; j++) {
        char c = filename[j];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        target_name[j] = c;
    }

    current_cluster = fs->cwd_cluster;
    entries_per_cluster =
        (fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster) / sizeof(DirEntry);

    while (!is_end_of_chain(current_cluster)) {
        offset = cluster_to_offset(fs, current_cluster);

        for (i = 0; i < entries_per_cluster; i++) {
            fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
            fread(&entry, sizeof(DirEntry), 1, fs->fp);

            if (entry.name[0] == 0x00) {
                return -1;
            }

            if (entry.name[0] == 0xE5 || entry.attr == ATTR_LONG_NAME) {
                continue;
            }

            format_name(entry.name, formatted_name);

            if (strcmp(formatted_name, target_name) == 0 ||
                strcmp(formatted_name, filename) == 0) {
                entry.first_cluster_high = (uint16_t)(first_cluster >> 16);
                entry.first_cluster_low = (uint16_t)(first_cluster & 0xFFFF);
                entry.file_size = file_size;

                fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
                fwrite(&entry, sizeof(DirEntry), 1, fs->fp);
                fflush(fs->fp);
                return 0;
            }
        }

        current_cluster = read_fat_entry(fs, current_cluster);
    }

    return -1;
}

static uint32_t get_nth_cluster(FileSystem *fs, uint32_t first_cluster, uint32_t n) {
    uint32_t current_cluster;
    uint32_t index;

    if (first_cluster < 2) {
        return 0;
    }

    current_cluster = first_cluster;

    for (index = 0; index < n; index++) {
        uint32_t next_cluster = read_fat_entry(fs, current_cluster);
        if (is_end_of_chain(next_cluster)) {
            return 0;
        }
        current_cluster = next_cluster;
    }

    return current_cluster;
}

static uint32_t append_new_cluster(FileSystem *fs, uint32_t last_cluster) {
    uint32_t new_cluster;

    new_cluster = find_free_cluster(fs);
    if (new_cluster == 0) {
        return 0;
    }

    write_fat_entry(fs, new_cluster, 0x0FFFFFF8);
    write_empty_cluster(fs, new_cluster);

    if (last_cluster >= 2) {
        write_fat_entry(fs, last_cluster, new_cluster);
    }

    return new_cluster;
}

static int ensure_file_capacity(FileSystem *fs, const char *filename,
                                uint32_t required_size,
                                uint32_t *first_cluster_out) {
    uint32_t first_cluster;
    uint32_t cluster_size;
    uint32_t required_clusters;
    uint32_t current_clusters;
    uint32_t current_cluster;
    uint32_t last_cluster;

    if (get_file_first_cluster(filename, &first_cluster) != 0) {
        return -1;
    }

    cluster_size = fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster;
    required_clusters = (required_size + cluster_size - 1) / cluster_size;

    if (required_clusters == 0) {
        *first_cluster_out = first_cluster;
        return 0;
    }

    if (first_cluster < 2) {
        first_cluster = find_free_cluster(fs);
        if (first_cluster == 0) {
            return -1;
        }

        write_fat_entry(fs, first_cluster, 0x0FFFFFF8);
        write_empty_cluster(fs, first_cluster);
        update_open_file_first_cluster(filename, first_cluster);
    }

    current_clusters = 1;
    current_cluster = first_cluster;

    while (1) {
        uint32_t next_cluster = read_fat_entry(fs, current_cluster);
        if (is_end_of_chain(next_cluster)) {
            last_cluster = current_cluster;
            break;
        }
        current_cluster = next_cluster;
        current_clusters++;
    }

    while (current_clusters < required_clusters) {
        uint32_t new_cluster = append_new_cluster(fs, last_cluster);
        if (new_cluster == 0) {
            return -1;
        }
        last_cluster = new_cluster;
        current_clusters++;
    }

    *first_cluster_out = first_cluster;
    return 0;
}

static void free_cluster_chain(const FileSystem *fs, uint32_t first_cluster) {
    uint32_t current_cluster;
    uint32_t next_cluster;

    if (first_cluster < 2) {
        return;
    }

    current_cluster = first_cluster;

    while (1) {
        next_cluster = read_fat_entry(fs, current_cluster);
        write_fat_entry(fs, current_cluster, 0x00000000);

        if (is_end_of_chain(next_cluster)) {
            break;
        }

        current_cluster = next_cluster;
    }
}

static int is_directory_empty(const FileSystem *fs, uint32_t dir_cluster) {
    uint32_t current_cluster;
    uint32_t offset;
    uint32_t entries_per_cluster;
    uint32_t i;
    DirEntry entry;
    char formatted_name[20];

    current_cluster = dir_cluster;
    entries_per_cluster =
        (fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster) / sizeof(DirEntry);

    while (!is_end_of_chain(current_cluster)) {
        offset = cluster_to_offset(fs, current_cluster);

        for (i = 0; i < entries_per_cluster; i++) {
            fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
            fread(&entry, sizeof(DirEntry), 1, fs->fp);

            if (entry.name[0] == 0x00) {
                return 1;
            }

            if (entry.name[0] == 0xE5 || entry.attr == ATTR_LONG_NAME) {
                continue;
            }

            format_name(entry.name, formatted_name);

            if (strcmp(formatted_name, ".") == 0 || strcmp(formatted_name, "..") == 0) {
                continue;
            }

            return 0;
        }

        current_cluster = read_fat_entry(fs, current_cluster);
    }

    return 1;
}

static int find_entry_location(const FileSystem *fs, uint32_t dir_cluster,
                               const char *name, DirEntry *result,
                               uint32_t *entry_offset_out) {
    uint32_t current_cluster;
    uint32_t offset;
    uint32_t entries_per_cluster;
    uint32_t i;
    DirEntry entry;
    char formatted_name[20];
    char target_name[20];
    size_t j;

    memset(target_name, 0, sizeof(target_name));
    for (j = 0; j < sizeof(target_name) - 1 && name[j] != '\0'; j++) {
        char c = name[j];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        target_name[j] = c;
    }

    current_cluster = dir_cluster;
    entries_per_cluster =
        (fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster) / sizeof(DirEntry);

    while (!is_end_of_chain(current_cluster)) {
        offset = cluster_to_offset(fs, current_cluster);

        for (i = 0; i < entries_per_cluster; i++) {
            uint32_t entry_offset = offset + (i * sizeof(DirEntry));

            fseek(fs->fp, entry_offset, SEEK_SET);
            fread(&entry, sizeof(DirEntry), 1, fs->fp);

            if (entry.name[0] == 0x00) {
                return -1;
            }

            if (entry.name[0] == 0xE5 || entry.attr == ATTR_LONG_NAME) {
                continue;
            }

            format_name(entry.name, formatted_name);

            if (strcmp(formatted_name, target_name) == 0 ||
                strcmp(formatted_name, name) == 0) {
                if (result != NULL) {
                    *result = entry;
                }
                if (entry_offset_out != NULL) {
                    *entry_offset_out = entry_offset;
                }
                return 0;
            }
        }

        current_cluster = read_fat_entry(fs, current_cluster);
    }

    return -1;
}

int mkdir_in_current_directory(FileSystem *fs, const char *dirname) {
    DirEntry existing;
    DirEntry new_entry;
    uint32_t free_cluster;
    uint32_t entry_offset;

    if (dirname == NULL || strlen(dirname) == 0 || strlen(dirname) > 11) {
        printf("Error: invalid directory name\n");
        return -1;
    }

    if (find_entry(fs, fs->cwd_cluster, dirname, &existing) == 0) {
        printf("Error: directory/file already exists\n");
        return -1;
    }

    free_cluster = find_free_cluster(fs);
    if (free_cluster == 0) {
        printf("Error: no free clusters available\n");
        return -1;
    }

    if (find_free_dir_entry(fs, fs->cwd_cluster, &entry_offset) != 0) {
        printf("Error: no free directory entry available\n");
        return -1;
    }

    write_fat_entry(fs, free_cluster, 0x0FFFFFF8);
    write_empty_cluster(fs, free_cluster);

    memset(&new_entry, 0, sizeof(DirEntry));
    make_83_name(dirname, new_entry.name);
    new_entry.attr = ATTR_DIRECTORY;
    new_entry.first_cluster_high = (uint16_t)(free_cluster >> 16);
    new_entry.first_cluster_low = (uint16_t)(free_cluster & 0xFFFF);
    new_entry.file_size = 0;

    fseek(fs->fp, entry_offset, SEEK_SET);
    fwrite(&new_entry, sizeof(DirEntry), 1, fs->fp);
    fflush(fs->fp);

    init_dot_entries(fs, free_cluster, fs->cwd_cluster);

    return 0;
}

int creat_in_current_directory(FileSystem *fs, const char *filename) {
    DirEntry existing;
    DirEntry new_entry;
    uint32_t entry_offset;

    if (filename == NULL || strlen(filename) == 0 || strlen(filename) > 11) {
        printf("Error: invalid file name\n");
        return -1;
    }

    if (find_entry(fs, fs->cwd_cluster, filename, &existing) == 0) {
        printf("Error: file/directory already exists\n");
        return -1;
    }

    if (find_free_dir_entry(fs, fs->cwd_cluster, &entry_offset) != 0) {
        printf("Error: no free directory entry available\n");
        return -1;
    }

    memset(&new_entry, 0, sizeof(DirEntry));
    make_83_name(filename, new_entry.name);
    new_entry.attr = 0x20;
    new_entry.first_cluster_high = 0;
    new_entry.first_cluster_low = 0;
    new_entry.file_size = 0;

    fseek(fs->fp, entry_offset, SEEK_SET);
    fwrite(&new_entry, sizeof(DirEntry), 1, fs->fp);
    fflush(fs->fp);

    return 0;
}

int read_file_data(FileSystem *fs, const char *filename, uint32_t size_to_read) {
    uint32_t file_offset;
    uint32_t file_size;
    uint32_t first_cluster;
    uint32_t cluster_size;
    uint32_t bytes_remaining;
    uint32_t bytes_actually_read;
    uint32_t current_cluster;
    uint32_t skip_clusters;
    uint32_t intra_cluster_offset;
    char mode[4];
    uint8_t buffer[1024];
    uint32_t bytes_to_read_now;

    if (get_file_mode(filename, mode, sizeof(mode)) != 0) {
        printf("Error: file is not open\n");
        return -1;
    }

    if (strcmp(mode, "-r") != 0 &&
        strcmp(mode, "-rw") != 0 &&
        strcmp(mode, "-wr") != 0) {
        printf("Error: file is not opened for reading\n");
        return -1;
    }

    if (get_file_offset(filename, &file_offset) != 0) {
        printf("Error: file is not open\n");
        return -1;
    }

    if (get_file_size_from_open_table(filename, &file_size) != 0) {
        printf("Error: file is not open\n");
        return -1;
    }

    if (get_file_first_cluster(filename, &first_cluster) != 0) {
        printf("Error: file is not open\n");
        return -1;
    }

    if (file_offset >= file_size) {
        printf("\n");
        return 0;
    }

    if (file_offset + size_to_read > file_size) {
        bytes_remaining = file_size - file_offset;
    } else {
        bytes_remaining = size_to_read;
    }

    bytes_actually_read = bytes_remaining;

    if (first_cluster < 2) {
        printf("\n");
        update_file_offset(filename, file_offset + bytes_actually_read);
        return 0;
    }

    cluster_size = fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster;
    current_cluster = first_cluster;
    skip_clusters = file_offset / cluster_size;
    intra_cluster_offset = file_offset % cluster_size;

    while (skip_clusters > 0) {
        current_cluster = read_fat_entry(fs, current_cluster);
        if (is_end_of_chain(current_cluster)) {
            printf("\n");
            update_file_offset(filename, file_offset);
            return 0;
        }
        skip_clusters--;
    }

    while (bytes_remaining > 0) {
        uint32_t cluster_offset;
        uint32_t available_in_cluster;

        cluster_offset = cluster_to_offset(fs, current_cluster) + intra_cluster_offset;
        available_in_cluster = cluster_size - intra_cluster_offset;

        bytes_to_read_now = bytes_remaining;
        if (bytes_to_read_now > available_in_cluster) {
            bytes_to_read_now = available_in_cluster;
        }
        if (bytes_to_read_now > sizeof(buffer)) {
            bytes_to_read_now = sizeof(buffer);
        }

        fseek(fs->fp, cluster_offset, SEEK_SET);
        fread(buffer, 1, bytes_to_read_now, fs->fp);
        fwrite(buffer, 1, bytes_to_read_now, stdout);

        bytes_remaining -= bytes_to_read_now;
        intra_cluster_offset = 0;

        if (bytes_remaining > 0) {
            current_cluster = read_fat_entry(fs, current_cluster);
            if (is_end_of_chain(current_cluster)) {
                break;
            }
        }
    }

    printf("\n");
    update_file_offset(filename, file_offset + bytes_actually_read);

    return 0;
}

int write_file_data(FileSystem *fs, const char *filename, const char *data) {
    uint32_t file_offset;
    uint32_t file_size;
    uint32_t first_cluster;
    uint32_t cluster_size;
    uint32_t data_len;
    uint32_t required_size;
    uint32_t current_cluster;
    uint32_t cluster_index;
    uint32_t intra_cluster_offset;
    uint32_t bytes_written;
    char mode[4];

    if (data == NULL) {
        printf("Error: invalid write data\n");
        return -1;
    }

    if (get_file_mode(filename, mode, sizeof(mode)) != 0) {
        printf("Error: file is not open\n");
        return -1;
    }

    if (strcmp(mode, "-w") != 0 &&
        strcmp(mode, "-rw") != 0 &&
        strcmp(mode, "-wr") != 0) {
        printf("Error: file is not opened for writing\n");
        return -1;
    }

    if (get_file_offset(filename, &file_offset) != 0) {
        printf("Error: file is not open\n");
        return -1;
    }

    if (get_file_size_from_open_table(filename, &file_size) != 0) {
        printf("Error: file is not open\n");
        return -1;
    }

    data_len = (uint32_t)strlen(data);
    required_size = file_offset + data_len;

    if (ensure_file_capacity(fs, filename, required_size, &first_cluster) != 0) {
        printf("Error: could not allocate clusters for write\n");
        return -1;
    }

    cluster_size = fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster;
    cluster_index = file_offset / cluster_size;
    intra_cluster_offset = file_offset % cluster_size;

    current_cluster = get_nth_cluster(fs, first_cluster, cluster_index);
    if (current_cluster == 0) {
        printf("Error: could not locate file cluster for write\n");
        return -1;
    }

    bytes_written = 0;

    while (bytes_written < data_len) {
        uint32_t writable_in_cluster;
        uint32_t bytes_to_write_now;
        uint32_t cluster_offset;

        writable_in_cluster = cluster_size - intra_cluster_offset;
        bytes_to_write_now = data_len - bytes_written;

        if (bytes_to_write_now > writable_in_cluster) {
            bytes_to_write_now = writable_in_cluster;
        }

        cluster_offset = cluster_to_offset(fs, current_cluster) + intra_cluster_offset;

        fseek(fs->fp, cluster_offset, SEEK_SET);
        fwrite(data + bytes_written, 1, bytes_to_write_now, fs->fp);

        bytes_written += bytes_to_write_now;
        intra_cluster_offset = 0;

        if (bytes_written < data_len) {
            current_cluster = read_fat_entry(fs, current_cluster);
            if (is_end_of_chain(current_cluster)) {
                printf("Error: cluster chain ended unexpectedly during write\n");
                return -1;
            }
        }
    }

    fflush(fs->fp);

    if (required_size > file_size) {
        file_size = required_size;
    }

    if (update_dir_entry_for_file(fs, filename, first_cluster, file_size) != 0) {
        printf("Error: could not update directory entry after write\n");
        return -1;
    }

    update_open_file_first_cluster(filename, first_cluster);
    update_open_file_size(filename, file_size);
    update_file_offset(filename, file_offset + data_len);

    return 0;
}

int remove_file_in_current_directory(FileSystem *fs, const char *filename) {
    uint32_t current_cluster;
    uint32_t offset;
    uint32_t entries_per_cluster;
    uint32_t i;
    DirEntry entry;
    char formatted_name[20];
    char target_name[20];
    size_t j;
    uint32_t first_cluster;

    if (filename == NULL) {
        printf("Error: invalid file name\n");
        return -1;
    }

    if (is_file_open(filename)) {
        printf("Error: file is open\n");
        return -1;
    }

    memset(target_name, 0, sizeof(target_name));
    for (j = 0; j < sizeof(target_name) - 1 && filename[j] != '\0'; j++) {
        char c = filename[j];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        target_name[j] = c;
    }

    current_cluster = fs->cwd_cluster;
    entries_per_cluster =
        (fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster) / sizeof(DirEntry);

    while (!is_end_of_chain(current_cluster)) {
        offset = cluster_to_offset(fs, current_cluster);

        for (i = 0; i < entries_per_cluster; i++) {
            fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
            fread(&entry, sizeof(DirEntry), 1, fs->fp);

            if (entry.name[0] == 0x00) {
                printf("Error: file does not exist\n");
                return -1;
            }

            if (entry.name[0] == 0xE5 || entry.attr == ATTR_LONG_NAME) {
                continue;
            }

            format_name(entry.name, formatted_name);

            if (strcmp(formatted_name, target_name) == 0 ||
                strcmp(formatted_name, filename) == 0) {

                if (is_directory(&entry)) {
                    printf("Error: target is a directory\n");
                    return -1;
                }

                first_cluster = get_entry_cluster(&entry);
                free_cluster_chain(fs, first_cluster);

                entry.name[0] = 0xE5;
                fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
                fwrite(&entry, sizeof(DirEntry), 1, fs->fp);
                fflush(fs->fp);

                return 0;
            }
        }

        current_cluster = read_fat_entry(fs, current_cluster);
    }

    printf("Error: file does not exist\n");
    return -1;
}

int remove_dir_in_current_directory(FileSystem *fs, const char *dirname) {
    uint32_t current_cluster;
    uint32_t offset;
    uint32_t entries_per_cluster;
    uint32_t i;
    DirEntry entry;
    char formatted_name[20];
    char target_name[20];
    size_t j;
    uint32_t dir_first_cluster;

    if (dirname == NULL) {
        printf("Error: invalid directory name\n");
        return -1;
    }

    memset(target_name, 0, sizeof(target_name));
    for (j = 0; j < sizeof(target_name) - 1 && dirname[j] != '\0'; j++) {
        char c = dirname[j];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        target_name[j] = c;
    }

    current_cluster = fs->cwd_cluster;
    entries_per_cluster =
        (fs->bs.bytes_per_sector * fs->bs.sectors_per_cluster) / sizeof(DirEntry);

    while (!is_end_of_chain(current_cluster)) {
        offset = cluster_to_offset(fs, current_cluster);

        for (i = 0; i < entries_per_cluster; i++) {
            fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
            fread(&entry, sizeof(DirEntry), 1, fs->fp);

            if (entry.name[0] == 0x00) {
                printf("Error: directory does not exist\n");
                return -1;
            }

            if (entry.name[0] == 0xE5 || entry.attr == ATTR_LONG_NAME) {
                continue;
            }

            format_name(entry.name, formatted_name);

            if (strcmp(formatted_name, target_name) == 0 ||
                strcmp(formatted_name, dirname) == 0) {

                if (!is_directory(&entry)) {
                    printf("Error: target is not a directory\n");
                    return -1;
                }

                dir_first_cluster = get_entry_cluster(&entry);

                if (!is_directory_empty(fs, dir_first_cluster)) {
                    printf("Error: directory is not empty\n");
                    return -1;
                }

                free_cluster_chain(fs, dir_first_cluster);

                entry.name[0] = 0xE5;
                fseek(fs->fp, offset + (i * sizeof(DirEntry)), SEEK_SET);
                fwrite(&entry, sizeof(DirEntry), 1, fs->fp);
                fflush(fs->fp);

                return 0;
            }
        }

        current_cluster = read_fat_entry(fs, current_cluster);
    }

    printf("Error: directory does not exist\n");
    return -1;
}

int move_entry_in_current_directory(FileSystem *fs, const char *source_name,
                                    const char *target_name) {
    DirEntry source_entry;
    DirEntry target_entry;
    uint32_t source_offset;
    uint32_t target_offset;
    uint32_t new_slot_offset;
    DirEntry moved_entry;

    if (source_name == NULL || target_name == NULL) {
        printf("Error: invalid mv arguments\n");
        return -1;
    }

    if (is_file_open(source_name)) {
        printf("Error: file must be closed before moving\n");
        return -1;
    }

    if (find_entry_location(fs, fs->cwd_cluster, source_name,
                            &source_entry, &source_offset) != 0) {
        printf("Error: source does not exist\n");
        return -1;
    }

    if (find_entry(fs, fs->cwd_cluster, target_name, &target_entry) == 0 &&
        is_directory(&target_entry)) {

        if (find_entry(fs, get_entry_cluster(&target_entry), source_name, NULL) == 0) {
            printf("Error: target directory already contains that name\n");
            return -1;
        }

        if (find_free_dir_entry(fs, get_entry_cluster(&target_entry), &new_slot_offset) != 0) {
            printf("Error: no free directory entry in target directory\n");
            return -1;
        }

        moved_entry = source_entry;

        fseek(fs->fp, new_slot_offset, SEEK_SET);
        fwrite(&moved_entry, sizeof(DirEntry), 1, fs->fp);

        source_entry.name[0] = 0xE5;
        fseek(fs->fp, source_offset, SEEK_SET);
        fwrite(&source_entry, sizeof(DirEntry), 1, fs->fp);
        fflush(fs->fp);

        return 0;
    }

    if (find_entry(fs, fs->cwd_cluster, target_name, NULL) == 0) {
        printf("Error: target name already exists\n");
        return -1;
    }

    if (strlen(target_name) == 0 || strlen(target_name) > 11) {
        printf("Error: invalid target name\n");
        return -1;
    }

    make_83_name(target_name, source_entry.name);

    fseek(fs->fp, source_offset, SEEK_SET);
    fwrite(&source_entry, sizeof(DirEntry), 1, fs->fp);
    fflush(fs->fp);

    return 0;
}