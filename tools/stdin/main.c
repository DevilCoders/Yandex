#include <stdio.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

int main(int argc, char** argv) {
    if (argc < 3)
        fputs("Usage: stdin INPUT_FILE PROGRAM ARGS...\n", stderr);
    else if (!freopen(argv[1], "r", stdin))
        perror("failed to open input file");
    else if (execvp(argv[2], &argv[2]))
        perror("failed to run program");
    return 1;
}
