#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

static volatile sig_atomic_t ready = 0;
static volatile sig_atomic_t val = 0;
static volatile sig_atomic_t done = 0;
static pid_t peer = -1;

static void h0(int sig){(void)sig;val=0;ready=1;if(peer>0)kill(peer,SIGUSR1);}
static void h1(int sig){(void)sig;val=1;ready=1;if(peer>0)kill(peer,SIGUSR1);}
static void hend(int sig){(void)sig;done=1;}

int main(void){
    setvbuf(stdout,NULL,_IONBF,0);
    printf("Receiver PID: %d\nEnter sender PID: ",getpid());
    if(scanf("%d",&peer)!=1)return 1;

    struct sigaction a0={0},a1={0},ad={0};
    a0.sa_handler=h0; sigaction(SIGUSR1,&a0,NULL);
    a1.sa_handler=h1; sigaction(SIGUSR2,&a1,NULL);
    ad.sa_handler=hend; sigaction(SIGINT,&ad,NULL);

    sigset_t blk,old,waitm;
    sigemptyset(&blk);
    sigaddset(&blk,SIGUSR1);sigaddset(&blk,SIGUSR2);sigaddset(&blk,SIGINT);
    sigprocmask(SIG_BLOCK,&blk,&old);
    waitm=old;
    sigdelset(&waitm,SIGUSR1);sigdelset(&waitm,SIGUSR2);sigdelset(&waitm,SIGINT);

    uint32_t acc=0;
    int idx=0;
    while(!done && idx<32){
        while(!ready && !done) sigsuspend(&waitm);
        if(done) break;
        if(val) acc|=(1u<<idx);
        idx++;
        ready=0;
    }
    printf("Received: %d\n",(int32_t)acc);
    sigprocmask(SIG_SETMASK,&old,NULL);
    return 0;
}

