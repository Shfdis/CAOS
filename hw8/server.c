#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "shared_data.h"

SharedData* shared_data = NULL;
int shm_fd = -1;

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

void cleanup(void) {
    if (shared_data != NULL) {
        shared_data->active = 0;
        shared_data->server_ts_ms = now_ms();
        usleep(200000);
        if (sem_wait(&shared_data->sem) == 0) {
            sem_post(&shared_data->sem);
            sem_destroy(&shared_data->sem);
        }
        munmap(shared_data, sizeof(SharedData));
    }
    if (shm_fd >= 0) {
        close(shm_fd);
    }
    shm_unlink(SHM_NAME);
}

int main(void) {
    atexit(cleanup);

    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        return 1;
    }

    shared_data = (SharedData*)mmap(NULL, sizeof(SharedData),
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return 1;
    }

    close(shm_fd);

    printf("[Сервер, PID: %d] Запущен. Ожидание данных...\n", getpid());
    shared_data->server_ts_ms = now_ms(); // первоначальный heartbeat

    while (shared_data->active) {
        long long now = now_ms();
        shared_data->server_ts_ms = now;

        if (sem_trywait(&shared_data->sem) == 0) {
            if (!shared_data->active) {
                sem_post(&shared_data->sem);
                break;
            }

            if (shared_data->ready == 1) {
                printf("[Сервер] Получено число: %d\n", shared_data->value);
                shared_data->ready = 0;
            }

            sem_post(&shared_data->sem);
        }

        usleep(50000);
    }

    printf("[Сервер, PID: %d] Завершение работы\n", getpid());

    return 0;
}
