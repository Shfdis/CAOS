#ifndef COORDINATOR_CORE_H
#define COORDINATOR_CORE_H

#include "tournament.h"
#include <semaphore.h>

int run_tournament(int n, MatchShared* match_data,
                   sem_t* sem_p1, sem_t* sem_p2, sem_t* sem_result, sem_t* sem_ready,
                   void (*notify)(const char* msg));

#endif

