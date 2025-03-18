#include <library/cpp/malloc/api/malloc.h>

using namespace NMalloc;

TMallocInfo NMalloc::MallocInfo() {
    TMallocInfo r;
    r.Name = "lockless";
    return r;
}
