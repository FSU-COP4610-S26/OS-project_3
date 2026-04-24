#include <stdio.h>
#include <string.h>
#include "open_table.h"

static OpenFile open_files[MAX_OPEN_FILES];

void init_open_table(void) {
    int i;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].used = 0;
        open_files[i].name[0] = '\0';
        open_files[i].mode[0] = '\0';
        open_files[i].path[0] = '\0';
        open_files[i].first_cluster = 0;
        open_files[i].size = 0;
        open_files[i].offset = 0;
    }
}

int is_valid_mode(const char *mode) {
    if (mode == NULL) {
        return 0;
    }

    return strcmp(mode, "-r") == 0 ||
           strcmp(mode, "-w") == 0 ||
           strcmp(mode, "-rw") == 0 ||
           strcmp(mode, "-wr") == 0;
}

int is_file_open(const char *filename) {
    int i;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            return 1;
        }
    }

    return 0;
}

int add_open_file(const char *filename, const char *mode,
                  const char *path, uint32_t first_cluster, uint32_t size) {
    int i;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].used) {
            open_files[i].used = 1;

            strncpy(open_files[i].name, filename, sizeof(open_files[i].name) - 1);
            open_files[i].name[sizeof(open_files[i].name) - 1] = '\0';

            strncpy(open_files[i].mode, mode, sizeof(open_files[i].mode) - 1);
            open_files[i].mode[sizeof(open_files[i].mode) - 1] = '\0';

            strncpy(open_files[i].path, path, sizeof(open_files[i].path) - 1);
            open_files[i].path[sizeof(open_files[i].path) - 1] = '\0';

            open_files[i].first_cluster = first_cluster;
            open_files[i].size = size;
            open_files[i].offset = 0;

            return 0;
        }
    }

    return -1;
}

int remove_open_file(const char *filename) {
    int i;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            open_files[i].used = 0;
            open_files[i].name[0] = '\0';
            open_files[i].mode[0] = '\0';
            open_files[i].path[0] = '\0';
            open_files[i].first_cluster = 0;
            open_files[i].size = 0;
            open_files[i].offset = 0;
            return 0;
        }
    }

    return -1;
}

void list_open_files(void) {
    int i;
    int found = 0;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used) {
            printf("%d %s %s %u %s\n",
                   i,
                   open_files[i].name,
                   open_files[i].mode,
                   open_files[i].offset,
                   open_files[i].path);
            found = 1;
        }
    }

    if (!found) {
        printf("No files are currently open\n");
    }
}

int set_file_offset(const char *filename, uint32_t new_offset) {
    int i;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            if (new_offset > open_files[i].size) {
                return -2;
            }

            open_files[i].offset = new_offset;
            return 0;
        }
    }

    return -1;
}

int get_file_offset(const char *filename, uint32_t *offset) {
    int i;

    if (offset == NULL) {
        return -1;
    }

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            *offset = open_files[i].offset;
            return 0;
        }
    }

    return -1;
}

int get_file_size_from_open_table(const char *filename, uint32_t *size) {
    int i;

    if (size == NULL) {
        return -1;
    }

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            *size = open_files[i].size;
            return 0;
        }
    }

    return -1;
}

int get_file_mode(const char *filename, char *mode_buffer, uint32_t mode_buffer_size) {
    int i;

    if (mode_buffer == NULL || mode_buffer_size == 0) {
        return -1;
    }

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            strncpy(mode_buffer, open_files[i].mode, mode_buffer_size - 1);
            mode_buffer[mode_buffer_size - 1] = '\0';
            return 0;
        }
    }

    return -1;
}

int get_file_first_cluster(const char *filename, uint32_t *first_cluster) {
    int i;

    if (first_cluster == NULL) {
        return -1;
    }

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            *first_cluster = open_files[i].first_cluster;
            return 0;
        }
    }

    return -1;
}

int update_file_offset(const char *filename, uint32_t new_offset) {
    int i;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            open_files[i].offset = new_offset;
            return 0;
        }
    }

    return -1;
}

int update_open_file_size(const char *filename, uint32_t new_size) {
    int i;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            open_files[i].size = new_size;
            return 0;
        }
    }

    return -1;
}

int update_open_file_first_cluster(const char *filename, uint32_t new_first_cluster) {
    int i;

    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].used && strcmp(open_files[i].name, filename) == 0) {
            open_files[i].first_cluster = new_first_cluster;
            return 0;
        }
    }

    return -1;
}