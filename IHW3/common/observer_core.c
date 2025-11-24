#include "observer_core.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

int observer_loop(const char *role, void *ctx, recv_fn recv) {
    setup_sigint(role);
    
    printf(" НАБЛЮДАТЕЛЬ ТУРНИРА \n");
    printf("[Наблюдатель, PID: %d] Ожидание данных...\n\n", getpid());
    
    char buffer[512];
    
    while (!g_stop) {
        ssize_t n = recv(ctx, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        } else if (n == 0) {
            break;
        } else {
            usleep(100000);
        }
    }
    
    printf("\n[Наблюдатель] Завершение работы\n");
    return 0;
}

