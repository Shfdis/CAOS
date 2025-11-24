#include "signals.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t g_stop = 0;

static void signal_handler(int sig) {
    if (sig == SIGINT) {
        g_stop = 1;
    }
}

void setup_sigint(const char *role) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    printf("\n[%s] Получен сигнал SIGINT. Завершение...\n", role);
}

