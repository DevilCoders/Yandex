#include <stdlib.h>
#include <malloc.h>

int main() {
    volatile char* ptr = new char[100];
    delete[] ptr;
    memalign(0, 0);
}
