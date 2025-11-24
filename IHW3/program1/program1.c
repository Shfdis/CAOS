#include "tournament.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>

volatile sig_atomic_t should_exit = 0;
MatchShared* match_shared = NULL;

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n[Родительский процесс %d] Получен сигнал SIGINT. Завершение всех процессов...\n", getpid());
        should_exit = 1;
        
        if (match_shared != NULL) {
            match_shared->active = 0;
        }
    }
}

void player_process(int player_id, MatchShared* match) {
    Game choice = random_choice();
    
    printf("[Игрок %d, PID: %d] Делаю выбор: %s\n", player_id, getpid(), game_to_string(choice));
    
    sem_wait(&match->sem);
    
    if (player_id == match->player1_id) {
        match->choice1 = choice;
        printf("[Игрок %d, PID: %d] Первый игрок, выбор сделан\n", player_id, getpid());
    } else if (player_id == match->player2_id) {
        match->choice2 = choice;
        printf("[Игрок %d, PID: %d] Второй игрок, выбор сделан\n", player_id, getpid());
    }
    
    sem_post(&match->sem);
}

int run_match(int player1, int player2, int match_id, MatchShared* match) {
    sem_wait(&match->sem);
    match->player1_id = player1;
    match->player2_id = player2;
    match->match_id = match_id;
    match->choice1 = Null;
    match->choice2 = Null;
    match->winner = 0;
    sem_post(&match->sem);
    
    int round = 1;
    int winner = 0;
    
    while (winner == 0 && !should_exit) {
        printf("[Раунд %d] Матч #%d: Игрок %d vs Игрок %d\n", round, match_id, player1, player2);
        
        pid_t pid1 = fork();
        if (pid1 == -1) {
            perror("fork failed");
            return -1;
        }
        
        if (pid1 == 0) {
            player_process(player1, match);
            _exit(0);
        }
        
        pid_t pid2 = fork();
        
        if (pid2 == 0) {
            player_process(player2, match);
            _exit(0);
        }
        
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        
        if (should_exit) {
            return -1;
        }
        
        sem_wait(&match->sem);
        Game choice1 = match->choice1;
        Game choice2 = match->choice2;
        
        printf("[Раунд %d] Игрок %d выбрал %s, Игрок %d выбрал %s\n",
               round, player1, game_to_string(choice1), player2, game_to_string(choice2));
        
        int result = decide_winner(choice1, choice2);
        
        if (result == 0) {
            printf("[Раунд %d] Ничья, переигровка\n", round);
            round++;
        } else if (result == 1) {
            winner = player1;
            printf("[Раунд %d] Победитель: Игрок %d\n", round, player1);
        } else {
            winner = player2;
            printf("[Раунд %d] Победитель: Игрок %d\n", round, player2);
        }
        
        match->winner = winner;
        sem_post(&match->sem);
    }
    
    if (!should_exit) {
        printf("[Матч #%d завершен] Игрок %d vs Игрок %d. Победитель - Игрок %d\n",
               match_id, player1, player2, winner);
    }
    
    return winner;
}

int main() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    printf(" Турнир 'Камень-Ножницы-Бумага' \n");
    printf("[Родительский процесс, PID: %d] Введите количество игроков: ", getpid());
    
    int n;
    if (scanf("%d", &n) != 1 || n < 1) {
        printf("Ошибка: неверное количество игроков\n");
        return 1;
    }
    
    printf("[Родительский процесс, PID: %d] Запуск турнира с %d игроками\n", getpid(), n);
    printf("Для завершения нажмите Ctrl+C\n\n");
    
    match_shared = (MatchShared*)mmap(NULL, sizeof(MatchShared),
                                       PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    sem_init(&match_shared->sem, 1, 1);
    
    match_shared->active = 1;
    
    int* players = (int*)malloc(n * sizeof(int));
    
    for (int i = 0; i < n; i++) {
        players[i] = i + 1;
    }
    
    int remaining = n;
    int match_id = 1;
    
    while (remaining > 1 && !should_exit) {
        printf("\n Раунд с %d игроками \n", remaining);
        
        int* next_round = (int*)malloc((remaining / 2 + 1) * sizeof(int));
        
        int next_count = 0;
        
        for (int i = 0; i < remaining - 1; i += 2) {
            if (should_exit) break;
            
            int player1 = players[i];
            int player2 = players[i + 1];
            
            int winner = run_match(player1, player2, match_id++, match_shared);
            
            if (should_exit || winner <= 0) {
                free(next_round);
                goto cleanup;
            }
            
            next_round[next_count++] = winner;
        }
        
        if (remaining % 2 == 1 && !should_exit) {
            next_round[next_count++] = players[remaining - 1];
            printf("[Игрок %d проходит автоматически (нечетное количество)]\n", players[remaining - 1]);
        }
        
        free(players);
        players = next_round;
        remaining = next_count;
    }
    
    if (!should_exit && remaining == 1) {
        printf("\n ТУРНИР ЗАВЕРШЕН \n");
        printf("ПОБЕДИТЕЛЬ: Игрок %d\n", players[0]);
    } else if (should_exit) {
        printf("\n[Родительский процесс, PID: %d] Турнир прерван пользователем\n", getpid());
    }
    
cleanup:
    if (match_shared != NULL) {
        sem_destroy(&match_shared->sem);
        munmap(match_shared, sizeof(MatchShared));
    }
    
    if (players != NULL) {
        free(players);
    }
    
    return should_exit ? 1 : 0;
}
