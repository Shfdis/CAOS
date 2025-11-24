#include "tournament.h"
#include "coordinator_core.h"
#include "ipc_utils.h"
#include "signals.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

void send_to_observers_mq(const char* message) {
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY | O_NONBLOCK);
    if (mq != (mqd_t)-1) {
        mq_send(mq, message, strlen(message) + 1, 0);
        mq_close(mq);
    }
}

int main() {
    setup_sigint("Координатор");
    
    printf(" Турнир 'Камень-Ножницы-Бумага' (Программа 4) \n");
    printf("[Координатор, PID: %d] Введите количество игроков: ", getpid());
    
    int n;
    if (scanf("%d", &n) != 1 || n < 1) {
        printf("Ошибка: неверное количество игроков\n");
        return 1;
    }
    
    mq_unlink(MQ_NAME);
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;
    attr.mq_curmsgs = 0;
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    mq_close(mq);
    
    MatchShared* match_data;
    sem_t *sem_p1, *sem_p2, *sem_result, *sem_ready;
    
    if (open_match_shared(&match_data, 1) < 0 ||
        open_match_sems(&sem_p1, &sem_p2, &sem_result, &sem_ready, 1) < 0) {
        unlink_all_ipc(0, 1);
        return 1;
    }
    
    printf("[Координатор, PID: %d] Турнир начат. Ожидание подключения игроков и наблюдателей...\n", getpid());
    print_instructions(n, "Координатор", "./build/program4_player", "./build/program4_observer");
    getchar();
    
    int result = run_tournament(n, match_data, sem_p1, sem_p2, sem_result, sem_ready, send_to_observers_mq);
    
    sem_wait(sem_result);
    match_data->active = 0;
    sem_post(sem_result);
    sem_post(sem_p1);
    sem_post(sem_p2);
    
    close_match_sems(sem_p1, sem_p2, sem_result, sem_ready);
    unlink_all_ipc(0, 1);
    return result;
}
