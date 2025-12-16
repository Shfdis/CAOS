#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define MAX_DEPTH 100
#define TEMP_DIR "symlink_test_dir"

int main(void)
{
    if (mkdir(TEMP_DIR, 0755) < 0 && errno != EEXIST) {
        const char msg[] = "Ошибка создания директории\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    if (chdir(TEMP_DIR) < 0) {
        const char msg[] = "Ошибка перехода в директорию\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    int fd = open("a", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        const char msg[] = "Ошибка создания файла 'a'\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }
    close(fd);

    int depth = 0;
    char prev_name[32] = "a";
    char curr_name[32];

    for (int i = 0; i < MAX_DEPTH; i++) {
        if (i == 0) {
            strcpy(curr_name, "aa");
        } else {
            snprintf(curr_name, sizeof(curr_name), "a%c", 'a' + i);
        }

        if (symlink(prev_name, curr_name) < 0) {
            const char msg[] = "Ошибка создания символьной связи\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }

        int test_fd = open(curr_name, O_RDONLY);
        if (test_fd < 0) {
            printf("%d\n", depth);
            break;
        }
        close(test_fd);

        depth++;
        strcpy(prev_name, curr_name);
    }

    chdir("..");
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEMP_DIR);
    system(cmd);

    return 0;
}

