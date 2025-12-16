#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <stdatomic.h>

#define MAX_P 1024
#define ROCK 0
#define SCISS 1
#define PAP 2

// Структура участника с атомарными переменными
typedef struct {
    int num;
    atomic_int choice;
    atomic_int active;
    pthread_t thr;
} Participant;

// Структура поединка с условными переменными
typedef struct {
    int p1;
    int p2;
    atomic_int rnd_done; // 0 - не завершен, 1 - завершен
    atomic_int ready_count;
    pthread_cond_t cond;
    pthread_mutex_t mut;
} Battle;

Participant participants[MAX_P];
Battle battles[MAX_P / 2];
int n_total;
atomic_int round_num;
atomic_int remaining_count;
atomic_int tournament_running;
FILE* logfile;
pthread_mutex_t print_lock;
pthread_mutex_t rand_lock; // Мьютекс для rand()

// Барьер для синхронизации начала раунда (все потоки + главный поток)
pthread_barrier_t round_barrier;

const char* names[] = {"Камень", "Ножницы", "Бумага"};

void log_print(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    
    pthread_mutex_lock(&print_lock);
    vprintf(fmt, ap);
    fflush(stdout); // Принудительный сброс буфера stdout
    va_end(ap);
    
    if (logfile) {
        va_start(ap, fmt);
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

void* participant_thread(void* arg) {
    int my_id = *(int*)arg;
    unsigned int rseed = time(NULL) ^ (my_id * 1000);
    free(arg); // Освобождаем память аргумента
    
    while (atomic_load(&tournament_running)) {
        // Ожидание начала раунда на барьере
        // Барьер отпускает, когда все N потоков + main дойдут до него
        int bar_res = pthread_barrier_wait(&round_barrier);
        if (bar_res != 0 && bar_res != PTHREAD_BARRIER_SERIAL_THREAD) {
             // ошибка или разрушение барьера при выходе
             break;
        }

        if (!atomic_load(&tournament_running)) {
            break;
        }
        
        // Если участник выбыл, он просто ждет следующего раунда (на барьере в начале цикла)
        if (!atomic_load(&participants[my_id].active)) {
            continue;
        }
        
        // Поиск своего поединка
        int current_left = atomic_load(&remaining_count);
        int battle_idx = -1;
        for (int j = 0; j < current_left / 2; j++) {
            if (battles[j].p1 == my_id || battles[j].p2 == my_id) {
                battle_idx = j;
                break;
            }
        }
        
        if (battle_idx == -1 || battle_idx >= current_left / 2) {
            continue;
        }
        
        int enemy = (battles[battle_idx].p1 == my_id) ? 
                    battles[battle_idx].p2 : battles[battle_idx].p1;
        
        // Сделали выбор
        pthread_mutex_lock(&rand_lock);
        atomic_store(&participants[my_id].choice, rand() % 3);
        pthread_mutex_unlock(&rand_lock);
        
        // Сообщаем о готовности
        pthread_mutex_lock(&battles[battle_idx].mut);
        atomic_fetch_add(&battles[battle_idx].ready_count, 1);
        
        // Если оба готовы, будим всех (второго участника и ожидающих)
        if (atomic_load(&battles[battle_idx].ready_count) == 2) {
            pthread_cond_broadcast(&battles[battle_idx].cond);
        } else {
            // Ждем пока второй будет готов
            while (atomic_load(&battles[battle_idx].ready_count) < 2 && 
                   atomic_load(&tournament_running)) {
                pthread_cond_wait(&battles[battle_idx].cond, &battles[battle_idx].mut);
            }
        }
        pthread_mutex_unlock(&battles[battle_idx].mut);
        
        if (!atomic_load(&tournament_running)) break;

        // Поединок обрабатывается главным потоком (или одним из участников).
        // В этой реализации пусть главный поток (или prepare_round цикл) обрабатывает логику выигрыша,
        // а участники ждут флага завершения rnd_done.
        // Это упрощает синхронизацию вывода и переигровок, чтобы не дублировать логику.
        // Но wait, в ТЗ сказано "моделирующее турнир... каждый студент - отдельный поток". 
        // Желательно чтобы логика игры была в потоках или они активно участвовали.
        // В program1 логика в потоке. Сделаем тут так же.

        // Чтобы избежать гонки кто обрабатывает, пусть обрабатывает тот у кого ID меньше? 
        // Или просто мьютекс.
        // Используем мьютекс поединка.
        
        pthread_mutex_lock(&battles[battle_idx].mut);
        
        // Проверяем, обработан ли уже поединок (rnd_done ставится в 1 когда результат зафиксирован)
        if (atomic_load(&battles[battle_idx].rnd_done) == 0) {
             // Первый кто захватил мьютекс после готовности обоих, проводит игру
             // Но нам нужно чтобы ОБА были готовы. Мы это уже проверили выше.
             
             // Проводим игру (с переигровками)
             int ch1 = atomic_load(&participants[battles[battle_idx].p1].choice);
             int ch2 = atomic_load(&participants[battles[battle_idx].p2].choice);
             
             // Вывод только один раз
             log_print("Раунд %d: Студент %d (%s) vs Студент %d (%s)\n",
                        atomic_load(&round_num), battles[battle_idx].p1, 
                        names[ch1],
                        battles[battle_idx].p2,
                        names[ch2]);
             
             int res = who_wins(ch1, ch2);

             while (res == 0 && atomic_load(&tournament_running)) {
                 log_print("Ничья! Переигровка...\n");
                 // Задержка для реалистичности
                 usleep(2000); // Уменьшено для тестов
                 
                 // Обновляем выбор с защитой мьютексом rand_lock
                 pthread_mutex_lock(&rand_lock);
                 atomic_store(&participants[battles[battle_idx].p1].choice, rand() % 3);
                 atomic_store(&participants[battles[battle_idx].p2].choice, rand() % 3);
                 pthread_mutex_unlock(&rand_lock);
                 
                 ch1 = atomic_load(&participants[battles[battle_idx].p1].choice);
                 ch2 = atomic_load(&participants[battles[battle_idx].p2].choice);
                 
                 log_print("Раунд %d (переигровка): Студент %d (%s) vs Студент %d (%s)\n",
                             atomic_load(&round_num), battles[battle_idx].p1, 
                             names[ch1],
                             battles[battle_idx].p2,
                             names[ch2]);
                 
                 res = who_wins(ch1, ch2);
             }
             
             if (res == 1) {
                 atomic_store(&participants[battles[battle_idx].p2].active, 0);
                 log_print("Победитель поединка: Студент %d\n", battles[battle_idx].p1);
             } else if (res == 2) {
                 atomic_store(&participants[battles[battle_idx].p1].active, 0);
                 log_print("Победитель поединка: Студент %d\n", battles[battle_idx].p2);
             }
             
             atomic_store(&battles[battle_idx].rnd_done, 1);
             pthread_cond_broadcast(&battles[battle_idx].cond);
        } else {
             // Если поединок уже обработан другим потоком, просто ждем (хотя мы уже знаем что done)
             // На всякий случай
             while (atomic_load(&battles[battle_idx].rnd_done) == 0 && atomic_load(&tournament_running)) {
                 pthread_cond_wait(&battles[battle_idx].cond, &battles[battle_idx].mut);
             }
        }
        pthread_mutex_unlock(&battles[battle_idx].mut);
        
        // Небольшая задержка перед следующим раундом
        usleep(1000);
    }
    
    return NULL;
}

void run_tournament() {
    atomic_store(&round_num, 0);

    while (atomic_load(&tournament_running)) {
        int active_list[MAX_P];
        int cnt = 0;
        
        // Сбор активных участников
        for (int i = 0; i < n_total; i++) {
            if (atomic_load(&participants[i].active)) {
                active_list[cnt++] = i;
            }
        }
        
        atomic_store(&remaining_count, cnt);
        
        if (cnt <= 1) {
            for (int i = 0; i < n_total; i++) {
                if (atomic_load(&participants[i].active)) {
                    log_print("\n=== ПОБЕДИТЕЛЬ ТУРНИРА: Студент %d ===\n", i);
                    atomic_store(&tournament_running, 0);
                    // Пропускаем всех через барьер, чтобы они завершились
                    pthread_barrier_wait(&round_barrier);
                    return;
                }
            }
            // Если вдруг никого нет
            atomic_store(&tournament_running, 0);
            pthread_barrier_wait(&round_barrier);
            return;
        }
        
        int current_r = atomic_fetch_add(&round_num, 1) + 1;
        log_print("\n=== Раунд %d: %d участников ===\n", current_r, cnt);
        
        // Подготовка поединков
        for (int i = 0; i < cnt / 2; i++) {
            battles[i].p1 = active_list[i * 2];
            battles[i].p2 = active_list[i * 2 + 1];
            atomic_store(&battles[i].rnd_done, 0);
            atomic_store(&battles[i].ready_count, 0);
            // Инициализация cond/mut
            pthread_cond_init(&battles[i].cond, NULL);
            pthread_mutex_init(&battles[i].mut, NULL);
        }
        
        // ЗАПУСК РАУНДА: Ждем барьер, чтобы все потоки синхронизировались и увидели новые данные
        pthread_barrier_wait(&round_barrier);
        
        // Главный поток ждет завершения всех поединков этого раунда
        // Можно поллингом, как в program1
        int finished = 0;
        while (!finished && atomic_load(&tournament_running)) {
            finished = 1;
            for (int i = 0; i < cnt / 2; i++) {
                if (atomic_load(&battles[i].rnd_done) == 0) {
                    finished = 0;
                    break;
                }
            }
            if (!finished) usleep(10000);
        }
        
        // Очистка ресурсов поединков
        for (int i = 0; i < cnt / 2; i++) {
            pthread_cond_destroy(&battles[i].cond);
            pthread_mutex_destroy(&battles[i].mut);
        }
        
        usleep(2000);
    }
    
    pthread_barrier_destroy(&round_barrier);
}

void sig_handler(int s) {
    atomic_store(&tournament_running, 0);
    log_print("\nПолучен сигнал прерывания. Завершение...\n");
    // Возможно нужно "протолкнуть" барьер, если потоки висят
    // Но pthread_barrier_wait не имеет прерывания.
    // В реальном приложении нужно более сложная остановка, 
    // но здесь достаточно флага и, возможно, exit(0) если всё висит.
    // Попробуем просто завершиться.
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
        if (f) {
            fscanf(f, "%d", &n);
            fclose(f);
        }
    }
    
    if (n < 2 || n > MAX_P) {
        printf("Использование: %s [N] [-c config_file] [-o output_file]\n", argv[0]);
        return 1;
    }
    
    if (out_path) {
        logfile = fopen(out_path, "w");
    }
    
    n_total = n;
    atomic_store(&tournament_running, 1);
    
    // Инициализация барьера: N участников + 1 (главный поток)
    pthread_barrier_init(&round_barrier, NULL, n_total + 1);
    
    pthread_mutex_init(&print_lock, NULL);
    pthread_mutex_init(&rand_lock, NULL);
    srand(time(NULL));
    
    for (int i = 0; i < n_total; i++) {
        participants[i].num = i;
        atomic_store(&participants[i].active, 1);
        atomic_store(&participants[i].choice, 0);
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&participants[i].thr, NULL, participant_thread, id);
    }
    
    log_print("=== Начало турнира. Участников: %d ===\n\n", n_total);
    
    run_tournament();
    
    for (int i = 0; i < n_total; i++) {
        pthread_join(participants[i].thr, NULL);
    }
    
    pthread_barrier_destroy(&round_barrier);
    pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&rand_lock);
    
    if (logfile) {
        fclose(logfile);
    }
    
    return 0;
}

