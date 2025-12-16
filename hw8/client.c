#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
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
}

int main(void) {
    atexit(cleanup);

    srand(time(NULL) ^ getpid());

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(SharedData)) < 0) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    shared_data = (SharedData*)mmap(NULL, sizeof(SharedData),
                                    PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    if (sem_init(&shared_data->sem, 1, 1) < 0) {
        perror("sem_init");
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    shared_data->value = 0;
    shared_data->ready = 0;
    shared_data->active = 1;
    long long now = now_ms();
    shared_data->server_ts_ms = now; // инициализация heartbeat сервера (будет обновляться сервером)
    shared_data->client_ts_ms = now;

    close(shm_fd);

    printf("[Клиент, PID: %d] Запущен. Генерация случайных чисел...\n", getpid());

    while (shared_data->active) {
        long long now_hb = now_ms();
        long long srv_ts = shared_data->server_ts_ms;
        // если сервер не обновлял heartbeat больше 2000 мс, выходим
        if (srv_ts > 0 && now_hb - srv_ts > 2000) {
            break;
        }

        if (sem_trywait(&shared_data->sem) == 0) {
            if (!shared_data->active) {
                sem_post(&shared_data->sem);
                break;
            }

            if (shared_data->ready == 0) {
                int value = MIN_VALUE + rand() % (MAX_VALUE - MIN_VALUE + 1);
                shared_data->value = value;
                shared_data->ready = 1;
                shared_data->client_ts_ms = now_ms();
                printf("[Клиент] Сгенерировано число: %d\n", value);
            }

            sem_post(&shared_data->sem);
        }

        usleep(50000);
    }

    printf("[Клиент, PID: %d] Завершение работы\n", getpid());

    return 0;
}
