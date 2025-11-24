#include "tournament.h"
#include "player_core.h"
#include "ipc_utils.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>

void send_to_observers_mq(const char* message) {
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY | O_NONBLOCK);
    if (mq != (mqd_t)-1) {
        mq_send(mq, message, strlen(message) + 1, 0);
        mq_close(mq);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Использование: %s <player_id>\n", argv[0]);
        return 1;
    }
    
    int player_id = atoi(argv[1]);
    if (player_id < 1) {
        printf("Ошибка: неверный ID игрока\n");
        return 1;
    }
    
    setup_sigint("Игрок");
    printf("[Игрок %d, PID: %d] Запущен. Ожидание матчей...\n", player_id, getpid());
    
    MatchShared* match_data;
    sem_t *sem_p1, *sem_p2, *sem_result, *sem_ready;
    
    if (open_match_shared(&match_data, 0) < 0 ||
        open_match_sems(&sem_p1, &sem_p2, &sem_result, &sem_ready, 0) < 0) {
        return 1;
    }
    
    player_loop(player_id, match_data, sem_p1, sem_p2, sem_result, sem_ready, send_to_observers_mq);
    
    printf("[Игрок %d, PID: %d] Завершение работы\n", player_id, getpid());
    close_match_sems(sem_p1, sem_p2, sem_result, sem_ready);
    return 0;
}
