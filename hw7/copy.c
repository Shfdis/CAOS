#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        const char msg[] = "Usage: ./copy <source> <destination>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    int src = open(argv[1], O_RDONLY);
    if (src < 0) {
        return 1;
    }

    struct stat st;
    if (fstat(src, &st) < 0) {
        close(src);
        return 1;
    }

    mode_t mode = st.st_mode & 07777;
    int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (dst < 0) {
        close(src);
        return 1;
    }

    fchmod(dst, mode);

    char buf[32];
    for (;;) {
        ssize_t r = read(src, buf, sizeof(buf));
        if (r == 0) {
            break;
        }
        if (r < 0) {
            close(src);
            close(dst);
            return 1;
        }

        ssize_t written = 0;
        while (written < r) {
            ssize_t w = write(dst, buf + written, (size_t)r - (size_t)written);
            if (w <= 0) {
                close(src);
                close(dst);
                return 1;
            }
            written += w;
        }
    }

    close(src);
    close(dst);
    return 0;
}


