#include <library/cpp/balloc_market/optional/operators.h>

#undef NDEBUG

#include <cstdlib>
#include <cstring>
#include <cassert>

namespace NCalloc {
    const char* GetAllocationOwner(void* ptr);
}

int main() {
    char* buf = (char*)malloc(100);
    assert(TStringBuf("libc") == NCalloc::GetAllocationOwner(buf));

    ThreadEnableBalloc();
    char* buf2 = (char*)malloc(100);
    assert(TStringBuf("b") == NCalloc::GetAllocationOwner(buf2));

    NMalloc::MallocInfo().SetParam("alloc", "b");
    char* buf3 = (char*)malloc(100);
    assert(TStringBuf("b") == NCalloc::GetAllocationOwner(buf3));

    NMalloc::MallocInfo().SetParam("alloc", "lf");
    char* buf4 = (char*)malloc(100);
    assert(TStringBuf("lf") == NCalloc::GetAllocationOwner(buf4));

    NMalloc::MallocInfo().SetParam("alloc", "libc");
    char* buf5 = (char*)malloc(100);
    assert(TStringBuf("libc") == NCalloc::GetAllocationOwner(buf5));

    NMalloc::MallocInfo().SetParam("alloc", "auto");
    char* buf6 = (char*)malloc(100);
    assert(TStringBuf("b") == NCalloc::GetAllocationOwner(buf6));

    free(buf);
    free(buf2);
    free(buf3);
    free(buf4);
    free(buf5);
    free(buf6);

    return 0;
}
