#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

static volatile sig_atomic_t ack = 0;
static pid_t peer;

static void h(int sig) { (void)sig; ack = 1; }

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Sender PID: %d\nEnter receiver PID: ", getpid());
    if (scanf("%d", &peer) != 1) return 1;
    int32_t v;
    printf("Enter integer to send: ");
    if (scanf("%" SCNd32, &v) != 1) return 1;

    struct sigaction sa = {0};
    sa.sa_handler = h;
    sigaction(SIGUSR1, &sa, NULL);

    sigset_t m, old, waitm;
    sigemptyset(&m); sigaddset(&m, SIGUSR1);
    sigprocmask(SIG_BLOCK, &m, &old);
    waitm = old; sigdelset(&waitm, SIGUSR1);

    for (int i = 0; i < 32; i++) {
        int bit = (v >> i) & 1;
        kill(peer, bit ? SIGUSR2 : SIGUSR1);
        ack = 0;
        while (!ack) sigsuspend(&waitm);
    }
    kill(peer, SIGINT);
    sigprocmask(SIG_SETMASK, &old, NULL);
    return 0;
}

