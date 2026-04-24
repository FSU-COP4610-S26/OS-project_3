CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
SRC = src/main.c src/fat32.c src/boot.c src/shell.c src/utils.c src/dir.c src/open_table.c
OUT = bin/filesys

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -rf bin/filesys

run: $(OUT)
	./$(OUT)