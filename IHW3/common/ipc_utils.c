#include "ipc_utils.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <mqueue.h>

int open_match_shared(MatchShared **match, int create) {
    int shm_fd;
    if (create) {
        shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            return -1;
        }
        ftruncate(shm_fd, sizeof(MatchShared));
    } else {
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shm_fd == -1) {
            return -1;
        }
    }
    
    *match = (MatchShared*)mmap(NULL, sizeof(MatchShared),
                                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (*match == MAP_FAILED) {
        return -1;
    }
    
    if (create) {
        memset(*match, 0, sizeof(MatchShared));
        (*match)->active = 1;
    }
    
    close(shm_fd);
    return 0;
}

int open_match_sems(sem_t **p1, sem_t **p2, sem_t **res, sem_t **ready, int create) {
    if (create) {
        *p1 = sem_open(SEM_PLAYER1_NAME, O_CREAT, 0666, 0);
        *p2 = sem_open(SEM_PLAYER2_NAME, O_CREAT, 0666, 0);
        *res = sem_open(SEM_RESULT_NAME, O_CREAT, 0666, 1);
        *ready = sem_open(SEM_READY_NAME, O_CREAT, 0666, 0);
    } else {
        *p1 = sem_open(SEM_PLAYER1_NAME, 0);
        *p2 = sem_open(SEM_PLAYER2_NAME, 0);
        *res = sem_open(SEM_RESULT_NAME, 0);
        *ready = sem_open(SEM_READY_NAME, 0);
    }
    
    if (*p1 == SEM_FAILED || *p2 == SEM_FAILED || *res == SEM_FAILED || *ready == SEM_FAILED) {
        return -1;
    }
    return 0;
}

void close_match_sems(sem_t *p1, sem_t *p2, sem_t *res, sem_t *ready) {
    sem_close(p1);
    sem_close(p2);
    sem_close(res);
    sem_close(ready);
}

void unlink_all_ipc(int unlink_fifo, int unlink_mq) {
    sem_unlink(SEM_PLAYER1_NAME);
    sem_unlink(SEM_PLAYER2_NAME);
    sem_unlink(SEM_RESULT_NAME);
    sem_unlink(SEM_READY_NAME);
    shm_unlink(SHM_NAME);
    if (unlink_fifo) {
        unlink(FIFO_NAME);
    }
    if (unlink_mq) {
        mq_unlink(MQ_NAME);
    }
}

