

#include "tournament.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static int run_match(int player1, int player2, int match_id,
                     MatchShared* match_data,
                     sem_t* sem_p1, sem_t* sem_p2, sem_t* sem_result, sem_t* sem_ready,
                     void (*notify)(const char* msg)) {
    if (notify) {
        char msg[256];
        snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Матч #%d: Игрок %d vs Игрок %d\n", 
                match_id, player1, player2);
        notify(msg);
    }
    
    printf("[Координатор] Матч #%d: Игрок %d vs Игрок %d\n", match_id, player1, player2);
    
    int winner = 0;
    int round = 1;
    
    while (winner == 0) {
        sem_wait(sem_result);
        match_data->player1_id = player1;
        match_data->player2_id = player2;
        match_data->match_id = match_id;
        match_data->choice1 = Null;
        match_data->choice2 = Null;
        match_data->winner = 0;
        sem_post(sem_result);
        
        sem_post(sem_p1);
        sem_post(sem_p2);cd 
        
        sem_wait(sem_ready);
        sem_wait(sem_ready);
        
        if (!match_data->active) {
            return -1;
        }
        
        Game choice1 = match_data->choice1;
        Game choice2 = match_data->choice2;
        
        printf("[Координатор] Матч #%d, Раунд %d: Игрок %d выбрал %s, Игрок %d выбрал %s\n",
               match_id, round, player1, game_to_string(choice1), 
               player2, game_to_string(choice2));
        
        if (notify) {
            char msg[256];
            snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Матч #%d, Раунд %d: %s vs %s\n",
                    match_id, round, game_to_string(choice1), game_to_string(choice2));
            notify(msg);
        }
        
        int result = decide_winner(choice1, choice2);
        
        if (result == 0) {
            winner = 0;
            printf("[Координатор] Ничья, переигровка\n");
            if (notify) {
                char msg[256];
                snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Матч #%d: Ничья, переигровка\n", match_id);
                notify(msg);
            }
            sem_post(sem_result);
            round++;
            usleep(500000);
        } else if (result == 1) {
            winner = player1;
        } else {
            winner = player2;
        }
        
        if (winner != 0) {
            sem_wait(sem_result);
            match_data->winner = winner;
            sem_post(sem_result);
            break;
        }
    }
    
    printf("[Координатор] Матч #%d завершен. Победитель: Игрок %d\n", match_id, winner);
    
    if (notify) {
        char msg[256];
        snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Матч #%d завершен. Победитель: Игрок %d\n",
                match_id, winner);
        notify(msg);
    }
    
    return winner;
}

int run_tournament(int n, MatchShared* match_data,
                   sem_t* sem_p1, sem_t* sem_p2, sem_t* sem_result, sem_t* sem_ready,
                   void (*notify)(const char* msg)) {
    if (notify) {
        char msg[256];
        snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Турнир начат с %d игроками\n", n);
        notify(msg);
    }
    
    int* players = (int*)malloc(n * sizeof(int));
    
    for (int i = 0; i < n; i++) {
        players[i] = i + 1;
    }
    
    int match_id = 1;
    int remaining = n;
    
    while (remaining > 1) {
        printf("\n[Координатор] Раунд с %d игроками\n", remaining);
        
        if (notify) {
            char msg[256];
            snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Начало раунда с %d игроками\n", remaining);
            notify(msg);
        }
        
        int* next_round = (int*)malloc((remaining / 2 + 1) * sizeof(int));
        
        int new_remaining = 0;
        
        for (int i = 0; i < remaining - 1; i += 2) {
            int player1 = players[i];
            int player2 = players[i + 1];
            
            int winner = run_match(player1, player2, match_id++, match_data,
                                   sem_p1, sem_p2, sem_result, sem_ready, notify);
            
            if (winner <= 0) {
                free(next_round);
                free(players);
                return 1;
            }
            
            next_round[new_remaining++] = winner;
        }
        
        if (remaining % 2 == 1) {
            next_round[new_remaining++] = players[remaining - 1];
            printf("[Координатор] Игрок %d проходит автоматически\n", players[remaining - 1]);
            if (notify) {
                char msg[256];
                snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] Игрок %d проходит автоматически\n", 
                        players[remaining - 1]);
                notify(msg);
            }
        }
        
        free(players);
        players = next_round;
        remaining = new_remaining;
    }
    
    if (remaining == 1) {
        printf("\n ТУРНИР ЗАВЕРШЕН \n");
        printf("ПОБЕДИТЕЛЬ: Игрок %d\n", players[0]);
        
        if (notify) {
            char msg[256];
            snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ]  ТУРНИР ЗАВЕРШЕН \n");
            notify(msg);
            snprintf(msg, sizeof(msg), "[НАБЛЮДАТЕЛЬ] ПОБЕДИТЕЛЬ: Игрок %d\n", players[0]);
            notify(msg);
        }
    }
    
    free(players);
    return 0;
}

