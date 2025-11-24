#include "tournament.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

const char* game_to_string(Game g) {
    switch(g) {
        case Rock: return "Камень";
        case Paper: return "Бумага";
        case Scissors: return "Ножницы";
        default: return "Неизвестно";
    }
}

int decide_winner(Game a, Game b) {
    if (a == b) return 0;
    if ((a + 2) % 3 == b) return 1;
    return -1;
}

Game random_choice(void) {
    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL) ^ getpid());
        seeded = 1;
    }
    return (Game)(rand() % 3);
}

