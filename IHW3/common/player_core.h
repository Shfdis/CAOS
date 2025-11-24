#ifndef PLAYER_CORE_H
#define PLAYER_CORE_H

#include "tournament.h"
#include <semaphore.h>

int player_loop(int player_id, MatchShared* match_data,
                sem_t* sem_p1, sem_t* sem_p2, sem_t* sem_result, sem_t* sem_ready,
                void (*notify)(const char* msg));

#endif

