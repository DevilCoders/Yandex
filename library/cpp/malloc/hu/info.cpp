#include <library/cpp/malloc/api/malloc.h>

using namespace NMalloc;

TMallocInfo NMalloc::MallocInfo() {
    TMallocInfo r;
    r.Name = "hu";
    return r;
}
