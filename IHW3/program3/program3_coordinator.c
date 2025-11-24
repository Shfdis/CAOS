#include "tournament.h"
#include "coordinator_core.h"
#include "ipc_utils.h"
#include "signals.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void send_to_observer_fifo(const char* message) {
    int fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
    if (fd != -1) {
        write(fd, message, strlen(message));
        close(fd);
    }
}

int main() {
    setup_sigint("Координатор");
    
    printf(" Турнир 'Камень-Ножницы-Бумага' (Программа 3) \n");
    printf("[Координатор, PID: %d] Введите количество игроков: ", getpid());
    
    int n;
    if (scanf("%d", &n) != 1 || n < 1) {
        printf("Ошибка: неверное количество игроков\n");
        return 1;
    }
    
    mkfifo(FIFO_NAME, 0666);
    
    MatchShared* match_data;
    sem_t *sem_p1, *sem_p2, *sem_result, *sem_ready;
    
    if (open_match_shared(&match_data, 1) < 0 ||
        open_match_sems(&sem_p1, &sem_p2, &sem_result, &sem_ready, 1) < 0) {
        unlink_all_ipc(1, 0);
        return 1;
    }
    
    printf("[Координатор, PID: %d] Турнир начат. Ожидание подключения игроков и наблюдателя...\n", getpid());
    print_instructions(n, "Координатор", "./build/program3_player", "./build/program3_observer");
    getchar();
    
    int result = run_tournament(n, match_data, sem_p1, sem_p2, sem_result, sem_ready, send_to_observer_fifo);
    
    sem_wait(sem_result);
    match_data->active = 0;
    sem_post(sem_result);
    sem_post(sem_p1);
    sem_post(sem_p2);
    
    close_match_sems(sem_p1, sem_p2, sem_result, sem_ready);
    unlink_all_ipc(1, 0);
    return result;
}
