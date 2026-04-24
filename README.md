# Project 3 - FAT32 File System (Amadou Landoure)

## Overview
This project implements a user-level file system interface in C that works with a FAT32 disk image. The program allows users to navigate directories and perform file operations such as creating, reading, writing, deleting, and moving files within the image.

The goal of this project is to understand how FAT32 works internally, including directory entries, cluster allocation, and file system navigation.

---

## Features

The program supports the following commands:

- `info` — Displays FAT32 file system information  
- `exit` — Exits the program  
- `cd [DIRNAME]` — Changes the current directory  
- `ls` — Lists contents of the current directory  
- `mkdir [DIRNAME]` — Creates a new directory  
- `creat [FILENAME]` — Creates a new file  
- `open [FILENAME] [FLAGS]` — Opens a file with a mode (`-r`, `-w`, `-rw`, `-wr`)  
- `close [FILENAME]` — Closes a file  
- `lsof` — Lists all open files  
- `lseek [FILENAME] [OFFSET]` — Moves the file offset  
- `read [FILENAME] [SIZE]` — Reads bytes from a file  
- `write [FILENAME] [STRING]` — Writes data to a file  
- `rm [FILENAME]` — Deletes a file  
- `rmdir [DIRNAME]` — Deletes an empty directory  
- `mv [SOURCE] [TARGET]` — Renames or moves a file/directory  

---

## Project Structure


Project3/
├── bin/
├── include/
├── src/
├── Makefile
└── README.md


- `src/` contains all implementation files  
- `include/` contains header files  
- `bin/` contains the compiled executable  
- `Makefile` is used to build the project  

---

## File Descriptions

- `main.c` — Entry point, initializes the file system  
- `shell.c` — Handles command parsing and execution  
- `boot.c` — Reads FAT32 boot sector information  
- `fat32.c` — Handles low-level FAT32 operations  
- `dir.c` — Implements directory and file operations (core logic)  
- `open_table.c` — Manages open file tracking  
- `utils.c` — Helper functions  

---

## Compilation

To compile the project, run:

```bash
make

This will generate:

bin/filesys
Running the Program

Run the program with a FAT32 image:

./bin/filesys fat32.img
Example Usage
info
ls
mkdir DIR1
cd DIR1
creat FILE1
open FILE1 -rw
write FILE1 "hello world"
lseek FILE1 0
read FILE1 11
close FILE1
cd ..
mv DIR1 DIR2
rm FILE1
rmdir DIR2
exit
Important Notes
Only FAT32 short 8.3 filenames are supported
Long filename entries are ignored
Files must be closed before moving or deleting
Directories must be empty before removal
Challenges Faced

One of the main challenges in this project was correctly managing cluster chains and ensuring consistency between directory entries and FAT table updates. Debugging issues related to file offsets and cluster allocation required careful tracing of how data is stored and accessed across clusters.

Handling edge cases such as empty directories, open file restrictions, and name formatting also required attention to detail.

What I Learned

Through this project, I gained a deeper understanding of:

How FAT32 organizes data using clusters and directory entries
How file systems manage reading, writing, and allocation
How low-level file operations interact with structured data
How to design and debug a multi-file C program
Author

Amadou Landoure
Florida State University
Computer Science Major

This project was completed individually.