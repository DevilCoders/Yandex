#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("bad args\n");
        return 0;
    }

    mode_t mode = 0644;
    int fd = open(argv[1], O_CREAT | O_RDWR, mode);
    fprintf(stderr, "fd=%d\n", fd);
    if (fd <= 0) {
        perror("error opening file");
        return 1;
    }

    const int bufSize = 1024 * 1024;
    size_t size = atoll(argv[2]) * bufSize;

    if (size == 0) {
        perror("bad size");
        return 2;
    }

    if (ftruncate(fd, size) < 0) {
        perror("ftruncate failed");
        return 3;
    }

    int align = 4096;
    char* buf = malloc(bufSize + align);
    char* aligned = (char*) ((1 + ((intptr_t) buf) / align) * align);

    for (int i = 0; i < bufSize; ++i) {
        aligned[i] = i % 2 ? 0 : 0xFF;
        //aligned[i] = i % 2 ? '0' : '1';
    }

    //fprintf(stderr, "%c\n", aligned[0]);
    //fprintf(stderr, "%c\n", aligned[1]);
    //return 0;

    size_t off = 0;
    size_t rem = bufSize;
    while (off < size) {
        int ret = pwrite(fd, aligned, rem, off + bufSize - rem);
        if (ret < 0) {
            fprintf(stderr, "code=%d\n", errno);
            perror("pwrite error");
            continue;
        }

        rem -= ret;
        if (rem == 0) {
            off += bufSize;
            rem = bufSize;
        }

        if (off % (100 * bufSize) == 0) {
            fprintf(stderr, "written %lu bytes out of %lu\n", off, size);
        }
    }

    if (fsync(fd) < 0) {
        perror("fsync failed");
        return 4;
    }

    if (close(fd) < 0) {
        perror("close failed");
        return 5;
    }

    return 0;
}
