#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("bad args\n");
        return 0;
    }

    int fd = open(argv[1], O_RDONLY | O_DIRECT);
    fprintf(stderr, "fd=%d\n", fd);
    if (fd <= 0) {
        perror("error opening file");
        return 1;
    }

    const int blockSize = 4096;

    char str[blockSize];
    for (int i = 0; i < blockSize; ++i) {
        str[i] = i % 2 ? 0 : 0xFF;
    }

    int align = 4096;
    const int bufSize = 1024 * 1024;
    char* buf = malloc(bufSize + align);
    char* aligned = (char*) ((1 + ((intptr_t) buf) / align) * align);

    size_t n = 0;

    size_t off = 0;
    size_t rem = bufSize;
    int len;
    while (len = read(fd, aligned + bufSize - rem, rem)) {
        if (len < 0) {
            fprintf(stderr, "code=%d\n", errno);
            perror("Error reading file");
            return 2;
        }

        if (len == 0) {
            break;
        }

        rem -= len;
        if (rem == 0) {
            for (size_t i = 0; i < bufSize; i += blockSize) {
                n += memcmp(str, aligned + i, blockSize) == 0;
            }

            off += bufSize;
            rem = bufSize;
        }

        if (off % (100 * bufSize) == 0) {
            fprintf(stderr, "read %lu bytes, n: %lu\n", off, n);
        }
    }

    close(fd);

    return 0;
}
