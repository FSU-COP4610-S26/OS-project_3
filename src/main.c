#include <stdio.h>
#include <stdlib.h>
#include "fat32.h"
#include "shell.h"

int main(int argc, char *argv[]) {
    FileSystem fs;

    if (argc != 2) {
        fprintf(stderr, "Usage: ./filesys <FAT32 image>\n");
        return 1;
    }

    if (fs_open(&fs, argv[1]) != 0) {
        fprintf(stderr, "Error: could not open FAT32 image '%s'\n", argv[1]);
        return 1;
    }

    run_shell(&fs);
    fs_close(&fs);

    return 0;
}