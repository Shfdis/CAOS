#include "utils.h"
#include <stdio.h>

void print_instructions(int n, const char *coord_name, const char *player_cmd, const char *observer_cmd) {
    if (observer_cmd) {
        printf("[%s] Запустите наблюдателя: %s\n", coord_name, observer_cmd);
    }
    printf("[%s] Запустите игроков в отдельных консолях:\n", coord_name);
    for (int i = 1; i <= n; i++) {
        printf("  %s %d\n", player_cmd, i);
    }
    printf("[%s] Нажмите Enter когда все будут готовы...\n", coord_name);
}

