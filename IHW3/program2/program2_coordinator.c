#include "tournament.h"
#include "coordinator_core.h"
#include "ipc_utils.h"
#include "signals.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    setup_sigint("Координатор");
    
    printf(" Турнир 'Камень-Ножницы-Бумага' (Программа 2) \n");
    printf("[Координатор, PID: %d] Введите количество игроков: ", getpid());
    
    int n;
    if (scanf("%d", &n) != 1 || n < 1) {
        printf("Ошибка: неверное количество игроков\n");
        return 1;
    }
    
    MatchShared* match_data;
    sem_t *sem_p1, *sem_p2, *sem_result, *sem_ready;
    
    if (open_match_shared(&match_data, 1) < 0 ||
        open_match_sems(&sem_p1, &sem_p2, &sem_result, &sem_ready, 1) < 0) {
        unlink_all_ipc(0, 0);
        return 1;
    }
    
    printf("[Координатор, PID: %d] Турнир начат. Ожидание подключения игроков...\n", getpid());
    print_instructions(n, "Координатор", "./build/program2_player", NULL);
    getchar();
    
    int result = run_tournament(n, match_data, sem_p1, sem_p2, sem_result, sem_ready, NULL);
    
    sem_wait(sem_result);
    match_data->active = 0;
    sem_post(sem_result);
    sem_post(sem_p1);
    sem_post(sem_p2);
    
    close_match_sems(sem_p1, sem_p2, sem_result, sem_ready);
    unlink_all_ipc(0, 0);
    return result;
}
