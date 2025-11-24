#ifndef OBSERVER_CORE_H
#define OBSERVER_CORE_H

#include <unistd.h>
#include <sys/types.h>

typedef ssize_t (*recv_fn)(void *ctx, char *buf, size_t size);

int observer_loop(const char *role, void *ctx, recv_fn recv);

#endif

