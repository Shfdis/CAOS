#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include "tournament.h"
#include <semaphore.h>

int open_match_shared(MatchShared **match, int create);
int open_match_sems(sem_t **p1, sem_t **p2, sem_t **res, sem_t **ready, int create);
void close_match_sems(sem_t *p1, sem_t *p2, sem_t *res, sem_t *ready);
void unlink_all_ipc(int unlink_fifo, int unlink_mq);

#endif

