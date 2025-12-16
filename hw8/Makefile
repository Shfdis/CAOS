CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE
LDFLAGS = -lrt -lpthread

all: client server

client: client.c shared_data.h
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

server: server.c shared_data.h
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

clean:
	rm -f client server

.PHONY: all clean

