#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "shell.h"
#include "boot.h"
#include "dir.h"
#include "open_table.h"
#include "utils.h"

#define MAX_INPUT 1024

static void print_prompt(const FileSystem *fs) {
    printf("%s%s> ", fs->image_name, fs->cwd_path);
}

static int extract_quoted_string(const char *input, char *output, size_t output_size) {
    const char *first_quote;
    const char *last_quote;
    size_t len;

    first_quote = strchr(input, '"');
    if (first_quote == NULL) {
        return -1;
    }

    last_quote = strrchr(input, '"');
    if (last_quote == NULL || last_quote == first_quote) {
        return -1;
    }

    len = (size_t)(last_quote - first_quote - 1);
    if (len >= output_size) {
        len = output_size - 1;
    }

    strncpy(output, first_quote + 1, len);
    output[len] = '\0';

    return 0;
}

void run_shell(FileSystem *fs) {
    char input[MAX_INPUT];
    char original_input[MAX_INPUT];
    char *command;
    char *arg1;
    char *arg2;

    init_open_table();

    while (1) {
        print_prompt(fs);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        trim_newline(input);

        if (strlen(input) == 0) {
            continue;
        }

        strncpy(original_input, input, sizeof(original_input) - 1);
        original_input[sizeof(original_input) - 1] = '\0';

        command = strtok(input, " ");
        arg1 = strtok(NULL, " ");
        arg2 = strtok(NULL, " ");

        if (command == NULL) {
            continue;
        }

        if (strcmp(command, "info") == 0) {
            print_info(fs->fp, &fs->bs);

        } else if (strcmp(command, "ls") == 0) {
            list_directory(fs, fs->cwd_cluster);

        } else if (strcmp(command, "cd") == 0) {
            DirEntry entry;
            uint32_t new_cluster;

            if (arg1 == NULL) {
                printf("Error: missing directory name\n");
                continue;
            }

            if (strcmp(arg1, "..") == 0) {
                fs->cwd_cluster = fs->bs.root_cluster;
                strcpy(fs->cwd_path, "/");
                continue;
            }

            if (find_entry(fs, fs->cwd_cluster, arg1, &entry) != 0) {
                printf("Error: directory not found\n");
                continue;
            }

            if (!is_directory(&entry)) {
                printf("Error: not a directory\n");
                continue;
            }

            new_cluster = get_entry_cluster(&entry);

            if (new_cluster < 2) {
                printf("Error: invalid directory cluster\n");
                continue;
            }

            fs->cwd_cluster = new_cluster;

            if (strcmp(fs->cwd_path, "/") == 0) {
                snprintf(fs->cwd_path, sizeof(fs->cwd_path), "/%s", arg1);
            } else {
                strncat(fs->cwd_path, "/", sizeof(fs->cwd_path) - strlen(fs->cwd_path) - 1);
                strncat(fs->cwd_path, arg1, sizeof(fs->cwd_path) - strlen(fs->cwd_path) - 1);
            }

        } else if (strcmp(command, "mkdir") == 0) {
            if (arg1 == NULL) {
                printf("Error: missing directory name\n");
                continue;
            }

            mkdir_in_current_directory(fs, arg1);

        } else if (strcmp(command, "creat") == 0) {
            if (arg1 == NULL) {
                printf("Error: missing file name\n");
                continue;
            }

            creat_in_current_directory(fs, arg1);

        } else if (strcmp(command, "open") == 0) {
            DirEntry entry;
            uint32_t first_cluster;

            if (arg1 == NULL || arg2 == NULL) {
                printf("Error: usage: open [FILENAME] [FLAGS]\n");
                continue;
            }

            if (!is_valid_mode(arg2)) {
                printf("Error: invalid mode\n");
                continue;
            }

            if (find_entry(fs, fs->cwd_cluster, arg1, &entry) != 0) {
                printf("Error: file does not exist\n");
                continue;
            }

            if (is_directory(&entry)) {
                printf("Error: cannot open a directory\n");
                continue;
            }

            if (is_file_open(arg1)) {
                printf("Error: file is already open\n");
                continue;
            }

            first_cluster = get_entry_cluster(&entry);

            if (add_open_file(arg1, arg2, fs->cwd_path, first_cluster, entry.file_size) != 0) {
                printf("Error: open file table is full\n");
                continue;
            }

        } else if (strcmp(command, "close") == 0) {
            if (arg1 == NULL) {
                printf("Error: missing file name\n");
                continue;
            }

            if (remove_open_file(arg1) != 0) {
                printf("Error: file is not open\n");
                continue;
            }

        } else if (strcmp(command, "lsof") == 0) {
            list_open_files();

        } else if (strcmp(command, "lseek") == 0) {
            unsigned long parsed_offset;
            int result;

            if (arg1 == NULL || arg2 == NULL) {
                printf("Error: usage: lseek [FILENAME] [OFFSET]\n");
                continue;
            }

            parsed_offset = strtoul(arg2, NULL, 10);
            result = set_file_offset(arg1, (uint32_t)parsed_offset);

            if (result == -1) {
                printf("Error: file is not open\n");
                continue;
            }

            if (result == -2) {
                printf("Error: offset is larger than file size\n");
                continue;
            }

        } else if (strcmp(command, "read") == 0) {
            unsigned long parsed_size;
            DirEntry entry;

            if (arg1 == NULL || arg2 == NULL) {
                printf("Error: usage: read [FILENAME] [SIZE]\n");
                continue;
            }

            if (find_entry(fs, fs->cwd_cluster, arg1, &entry) != 0) {
                printf("Error: file does not exist\n");
                continue;
            }

            if (is_directory(&entry)) {
                printf("Error: cannot read a directory\n");
                continue;
            }

            parsed_size = strtoul(arg2, NULL, 10);
            read_file_data(fs, arg1, (uint32_t)parsed_size);

        } else if (strcmp(command, "write") == 0) {
            char write_data[MAX_INPUT];
            DirEntry entry;

            if (arg1 == NULL) {
                printf("Error: usage: write [FILENAME] [STRING]\n");
                continue;
            }

            if (find_entry(fs, fs->cwd_cluster, arg1, &entry) != 0) {
                printf("Error: file does not exist\n");
                continue;
            }

            if (is_directory(&entry)) {
                printf("Error: cannot write to a directory\n");
                continue;
            }

            if (extract_quoted_string(original_input, write_data, sizeof(write_data)) != 0) {
                printf("Error: string must be enclosed in quotes\n");
                continue;
            }

            write_file_data(fs, arg1, write_data);

        } else if (strcmp(command, "rm") == 0) {
            if (arg1 == NULL) {
                printf("Error: missing file name\n");
                continue;
            }

            remove_file_in_current_directory(fs, arg1);

        } else if (strcmp(command, "rmdir") == 0) {
            if (arg1 == NULL) {
                printf("Error: missing directory name\n");
                continue;
            }

            remove_dir_in_current_directory(fs, arg1);

        } else if (strcmp(command, "mv") == 0) {
            if (arg1 == NULL || arg2 == NULL) {
                printf("Error: usage: mv [SOURCE] [TARGET]\n");
                continue;
            }

            move_entry_in_current_directory(fs, arg1, arg2);

        } else if (strcmp(command, "exit") == 0) {
            break;

        } else {
            printf("Error: unsupported command\n");
        }
    }
}