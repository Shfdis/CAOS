#include "tournament.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int player_loop(int player_id, MatchShared* match_data,
                sem_t* sem_p1, sem_t* sem_p2, sem_t* sem_result, sem_t* sem_ready,
                void (*notify)(const char* msg)) {
    if (notify) {
        char msg[256];
        snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Игрок %d (PID: %d) подключился\n", 
                player_id, getpid());
        notify(msg);
    }
    
    while (match_data->active) {
        sem_t* my_sem = NULL;
        if (player_id == match_data->player1_id) {
            my_sem = sem_p1;
        } else if (player_id == match_data->player2_id) {
            my_sem = sem_p2;
        }
        
        if (my_sem == NULL) {
            usleep(100000);
            continue;
        }
        
        if (sem_wait(my_sem) == 0) {
            if (!match_data->active) break;
            
            Game choice = random_choice();
            
            sem_wait(sem_result);
            
            if (player_id == match_data->player1_id) {
                match_data->choice1 = choice;
                printf("[Игрок %d, PID: %d] Матч #%d: Мой выбор - %s\n", 
                       player_id, getpid(), match_data->match_id, game_to_string(choice));
                if (notify) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Игрок %d выбрал %s в матче #%d\n",
                            player_id, game_to_string(choice), match_data->match_id);
                    notify(msg);
                }
            } else if (player_id == match_data->player2_id) {
                match_data->choice2 = choice;
                printf("[Игрок %d, PID: %d] Матч #%d: Мой выбор - %s\n", 
                       player_id, getpid(), match_data->match_id, game_to_string(choice));
                if (notify) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Игрок %d выбрал %s в матче #%d\n",
                            player_id, game_to_string(choice), match_data->match_id);
                    notify(msg);
                }
            }
            
            sem_post(sem_result);
            sem_post(sem_ready);
            
            usleep(500000);
            
            sem_wait(sem_result);
            if (match_data->winner == player_id) {
                printf("[Игрок %d, PID: %d] Победил в матче #%d!\n", 
                       player_id, getpid(), match_data->match_id);
                if (notify) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Игрок %d победил в матче #%d\n",
                            player_id, match_data->match_id);
                    notify(msg);
                }
            } else if (match_data->winner > 0) {
                printf("[Игрок %d, PID: %d] Проиграл в матче #%d\n", 
                       player_id, getpid(), match_data->match_id);
            }
            sem_post(sem_result);
        }
    }
    
    if (notify) {
        char msg[256];
        snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Игрок %d завершил работу\n", player_id);
        notify(msg);
    }
    
    return 0;
}

