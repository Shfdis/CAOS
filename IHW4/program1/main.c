#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

#define MAX_P 1024
#define ROCK 0
#define SCISS 1
#define PAP 2

typedef struct {
    int num;
    int ch;
    int alive;
    pthread_t thr;
} Student;

typedef struct {
    int p1;
    int p2;
    int rnd;
    int done;
    sem_t ready;
} Duel;

Student stud[MAX_P];
Duel duels[MAX_P / 2];
int n_total;
int round_num;
int left;
FILE* logfile;
pthread_mutex_t print_lock;
pthread_mutex_t game_lock;
sem_t start_round;
int running = 1;

const char* names[] = {"Камень", "Ножницы", "Бумага"};

void log_print(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    
    pthread_mutex_lock(&print_lock);
    vprintf(fmt, ap);
    va_end(ap); // Завершаем первое использование
    
    if (logfile) {
        va_start(ap, fmt); // Переинициализируем для второго использования
        vfprintf(logfile, fmt, ap);
        fflush(logfile);
        va_end(ap);
    }
    pthread_mutex_unlock(&print_lock);
}

int who_wins(int c1, int c2) {
    if (c1 == c2) return 0;
    if ((c1 == ROCK && c2 == SCISS) ||
        (c1 == SCISS && c2 == PAP) ||
        (c1 == PAP && c2 == ROCK)) {
        return 1;
    }
    return 2;
}

void* student_func(void* arg) {
    int my_id = *(int*)arg;
    unsigned int rseed = time(NULL) ^ (my_id * 1000);
    
    while (running) {
        sem_wait(&start_round);
        
        if (!running) {
            sem_post(&start_round);
            break;
        }
        
        pthread_mutex_lock(&game_lock);
        if (!stud[my_id].alive) {
            pthread_mutex_unlock(&game_lock);
            sem_post(&start_round);
            continue;
        }
        
        int current_left = left;
        int duel_idx = -1;
        for (int j = 0; j < current_left / 2; j++) {
            if (duels[j].p1 == my_id || duels[j].p2 == my_id) {
                duel_idx = j;
                break;
            }
        }
        
        if (duel_idx == -1 || duel_idx >= current_left / 2) {
            pthread_mutex_unlock(&game_lock);
            sem_post(&start_round);
            continue;
        }
        
        int enemy = (duels[duel_idx].p1 == my_id) ? 
                    duels[duel_idx].p2 : duels[duel_idx].p1;
        
        stud[my_id].ch = rand_r(&rseed) % 3;
        duels[duel_idx].done++;
        int done_val = duels[duel_idx].done;
        
        pthread_mutex_unlock(&game_lock);
        
        while (done_val < 2 && running) {
            usleep(1000);
            pthread_mutex_lock(&game_lock);
            done_val = duels[duel_idx].done;
            pthread_mutex_unlock(&game_lock);
        }
        
        if (!running) break;
        
        pthread_mutex_lock(&game_lock);
        
        if (duels[duel_idx].done == 2 && !duels[duel_idx].rnd) {
            int ch1 = stud[duels[duel_idx].p1].ch;
            int ch2 = stud[duels[duel_idx].p2].ch;
            
            log_print("Раунд %d: Студент %d (%s) vs Студент %d (%s)\n",
                        round_num, duels[duel_idx].p1, 
                        names[ch1],
                        duels[duel_idx].p2,
                        names[ch2]);
            
            int res = who_wins(ch1, ch2);
            
            while (res == 0 && running) {
                log_print("Ничья! Переигровка...\n");
                usleep(200000);
                
                rseed = time(NULL) ^ (my_id * 1000) ^ (rand() % 10000);
                stud[my_id].ch = rand_r(&rseed) % 3;
                
                if (my_id == duels[duel_idx].p1) {
                    ch1 = stud[my_id].ch;
                    ch2 = stud[enemy].ch;
                } else {
                    ch1 = stud[enemy].ch;
                    ch2 = stud[my_id].ch;
                }
                
                log_print("Раунд %d (переигровка): Студент %d (%s) vs Студент %d (%s)\n",
                            round_num, duels[duel_idx].p1, 
                            names[ch1],
                            duels[duel_idx].p2,
                            names[ch2]);
                
                res = who_wins(ch1, ch2);
            }
            
            if (res == 1) {
                stud[duels[duel_idx].p2].alive = 0;
                log_print("Победитель поединка: Студент %d\n", duels[duel_idx].p1);
            } else if (res == 2) {
                stud[duels[duel_idx].p1].alive = 0;
                log_print("Победитель поединка: Студент %d\n", duels[duel_idx].p2);
            }
            
            duels[duel_idx].rnd = 1;
        }
        
        pthread_mutex_unlock(&game_lock);
        
        sem_post(&start_round);
        usleep(100000);
    }
    
    return NULL;
}

void setup_round() {
    int active_list[MAX_P];
    int cnt = 0;
    
    for (int i = 0; i < n_total; i++) {
        if (stud[i].alive) {
            active_list[cnt++] = i;
        }
    }
    
    left = cnt;
    round_num++;
    
    if (left <= 1) {
        for (int i = 0; i < n_total; i++) {
            if (stud[i].alive) {
                log_print("\n=== ПОБЕДИТЕЛЬ ТУРНИРА: Студент %d ===\n", i);
                running = 0;
                return;
            }
        }
    }
    
    log_print("\n=== Раунд %d: %d участников ===\n", round_num, left);
    
    for (int i = 0; i < left / 2; i++) {
        duels[i].p1 = active_list[i * 2];
        duels[i].p2 = active_list[i * 2 + 1];
        duels[i].rnd = 0;
        duels[i].done = 0;
        sem_init(&duels[i].ready, 0, 0);
    }
    
    for (int i = 0; i < n_total; i++) {
        sem_post(&start_round);
    }
    
    usleep(500000);
    
    int finished = 0;
    while (!finished && running) {
        finished = 1;
        for (int i = 0; i < left / 2; i++) {
            if (duels[i].rnd == 0) {
                finished = 0;
                break;
            }
        }
        usleep(10000);
    }
    
    for (int i = 0; i < left / 2; i++) {
        sem_destroy(&duels[i].ready);
    }
    
    usleep(300000);
}

void sig_handler(int s) {
    running = 0;
    log_print("\nПолучен сигнал прерывания. Завершение турнира...\n");
}

int main(int argc, char* argv[]) {
    signal(SIGINT, sig_handler);
    
    char* cfg_path = NULL;
    char* out_path = NULL;
    int n = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            cfg_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (n == 0) {
            n = atoi(argv[i]);
        }
    }
    
    if (cfg_path) {
        FILE* f = fopen(cfg_path, "r");
        fscanf(f, "%d", &n);
        fclose(f);
    }
    
    if (n < 2 || n > MAX_P) {
        printf("Использование: %s [N] [-c config_file] [-o output_file]\n", argv[0]);
        return 1;
    }
    
    if (out_path) {
        logfile = fopen(out_path, "w");
    }
    
    n_total = n;
    round_num = 0;
    left = n;
    
    pthread_mutex_init(&print_lock, NULL);
    pthread_mutex_init(&game_lock, NULL);
    sem_init(&start_round, 0, 0);
    
    srand(time(NULL));
    
    for (int i = 0; i < n_total; i++) {
        stud[i].num = i;
        stud[i].alive = 1;
        stud[i].ch = 0;
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&stud[i].thr, NULL, student_func, id);
    }
    
    log_print("=== Начало турнира. Участников: %d ===\n\n", n_total);
    
    while (left > 1 && running) {
        setup_round();
    }
    
    running = 0;
    
    for (int i = 0; i < n_total; i++) {
        sem_post(&start_round);
    }
    
    for (int i = 0; i < n_total; i++) {
        pthread_join(stud[i].thr, NULL);
    }
    
    sem_destroy(&start_round);
    pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&game_lock);
    
    if (logfile) {
        fclose(logfile);
    }
    
    return 0;
}
