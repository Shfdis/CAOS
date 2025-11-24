#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include <semaphore.h>

#define SHM_NAME "/tournament_shm"
#define SEM_PLAYER1_NAME "/tournament_sem_p1"
#define SEM_PLAYER2_NAME "/tournament_sem_p2"
#define SEM_RESULT_NAME "/tournament_sem_result"
#define SEM_READY_NAME "/tournament_sem_ready"
#define FIFO_NAME "/tmp/tournament_fifo"
#define MQ_NAME "/tournament_mq"

typedef enum {
    Rock = 0,
    Paper = 1,
    Scissors = 2,
    Null = 3
} Game;

typedef struct {
    sem_t sem;
    int winner;
    Game choice1;
    Game choice2;
    int player1_id;
    int player2_id;
    int match_id;
    int active;
} MatchShared;

const char* game_to_string(Game g);
int decide_winner(Game a, Game b);
Game random_choice(void);

#endif

