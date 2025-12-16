CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: symlink_depth

symlink_depth: symlink_depth.c
	$(CC) $(CFLAGS) -o symlink_depth symlink_depth.c

clean:
	rm -f symlink_depth

.PHONY: all clean

