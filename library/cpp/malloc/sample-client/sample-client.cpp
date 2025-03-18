#include <util/stream/output.h>

#include <library/cpp/malloc/api/malloc.h>

int main() {
    Cout << "using malloc: " << NMalloc::MallocInfo().Name << "\n";
    return 0;
}
