#include "observer_core.h"
#include "tournament.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static ssize_t fifo_recv(void *ctx, char *buf, size_t size) {
    int fd = *(int*)ctx;
    return read(fd, buf, size);
}

int main() {
    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open fifo failed");
        printf("Убедитесь, что координатор запущен первым\n");
        return 1;
    }
    
    return observer_loop("Наблюдатель", &fd, fifo_recv);
}
