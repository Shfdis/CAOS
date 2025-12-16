#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <semaphore.h>

#define SHM_NAME "/hw8_shm"
#define SEM_NAME "/hw8_sem"

#define MIN_VALUE 0
#define MAX_VALUE 100

typedef struct {
    int value;
    int ready;               // 1 - данные готовы, 0 - данные прочитаны
    volatile int active;     // 1 - работа продолжается, 0 - завершение
    volatile long long server_ts_ms; // heartbeat сервера (ms)
    volatile long long client_ts_ms; // heartbeat клиента (ms)
    sem_t sem;               // Семафор для синхронизации
} SharedData;

#endif

