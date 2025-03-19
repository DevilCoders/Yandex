#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>

#define BUFSIZE 65536

int HUPPED = 1;

void hup_handler(int signo)
{
    HUPPED = 1;
}

int main(int argc, char** argv)
{
    char buf[BUFSIZE];
    int file = -1;

    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <filename>\n", argv[0]);
        return -1;
    }

    signal(SIGHUP, hup_handler);

    while(1) {
        if (HUPPED) {
            close(file);
            file = open(argv[1], O_CREAT | O_APPEND | O_WRONLY, 0644);
            HUPPED = 0;
        }

        ssize_t read_len = read(0, buf, BUFSIZE);
        if (!read_len) break;

        ssize_t written = 0;
        while (written < read_len) {
            written += write(file, buf + written, read_len - written);
        }
    }
}
