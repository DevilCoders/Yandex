#include <library/cpp/balloc_market/lib/balloc.h>
#include <library/cpp/lfalloc/lf_allocX64.h>
#include <library/cpp/malloc/calloc/options/options.h>

#include <util/system/guard.h>
#include <util/generic/map.h>
#include <util/generic/scope.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/system/compiler.h>
#include <util/system/mutex.h>

#include <new>
#include <pthread.h>
#include <sys/mman.h>

namespace NCalloc {
    static __thread EAllocType AllocType = EAllocType::NotInit;
    thread_local bool InsideAllocator = false;
#ifdef CALLOC_PROFILER
    thread_local ui8 threadLocalStackTraceSize;
    thread_local ui8 threadLocalStackTraceBuffer[STACK_TRACE_BUFFER_SIZE];
#endif

    static Y_FORCE_INLINE void Init() {
        if (Y_UNLIKELY(AllocType == EAllocType::NotInit)) {
            AllocType = EAllocType::Mmap;
            NMarket::NBalloc::Init();
            AllocType = EnabledByDefault ? DefaultAlloc : DefaultSlaveAlloc;
        }
    }

    static Y_FORCE_INLINE void* Allocate(size_t size) {
        Init();
        TAllocHeader::CollectStackTrace();
        InsideAllocator = true;
        Y_DEFER {
            InsideAllocator = false;
        };
        TAllocHeader* header;
        if (AllocType == EAllocType::BAlloc) {
            header = NMarket::NBalloc::AllocateRaw(size, AllocType);
        } else {
            size = NMarket::NBalloc::AlignUp(size, ALLOC_HEADER_ALIGNMENT);
            switch (AllocType) {
            case EAllocType::LF:
#ifndef _darwin_
                header = (TAllocHeader*)::LFAlloc(size + TAllocHeader::GetHeaderSize());
#else
                header = (TAllocHeader*)NMarket::NBalloc::LibcMalloc(size + TAllocHeader::GetHeaderSize());
#endif
                break;
#ifndef _musl_
            case EAllocType::System:
                header = (TAllocHeader*)NMarket::NBalloc::LibcMalloc(size + TAllocHeader::GetHeaderSize());
                break;
#endif
            case EAllocType::Mmap:
                size += TAllocHeader::GetHeaderSize();
                header = (TAllocHeader*)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
                break;
            case EAllocType::NotInit:
                NMalloc::AbortFromCorruptedAllocator();
            }
            header->Encode(header, size, AllocType);
        }
        return header->GetDataPtr();
    }

    static Y_FORCE_INLINE void Free(void* ptr) {
        if (!ptr) {
            return;
        }
        Init();
        TAllocHeader::Clear(ptr);
        InsideAllocator = true;
        Y_DEFER {
            InsideAllocator = false;
        };
        const size_t allocSize = TAllocHeader::GetAllocSize(ptr);
        switch (TAllocHeader::GetAllocType(ptr)) {
        case EAllocType::BAlloc:
            NMarket::NBalloc::FreeRaw(TAllocHeader::GetBlock(ptr));
            if (NMarket::NAllocStats::IsEnabled()) {
                NMarket::NAllocStats::DecThreadAllocStats(allocSize);
            }
            break;
        case EAllocType::LF:
#ifndef _darwin_
            ::LFFree(TAllocHeader::GetBlock(ptr));
#else
            NMarket::NBalloc::LibcFree(TAllocHeader::GetBlock(ptr));
#endif
            break;
#ifndef _musl_
        case EAllocType::System:
            NMarket::NBalloc::LibcFree(TAllocHeader::GetBlock(ptr));
            break;
#endif
        case EAllocType::Mmap:
            munmap(TAllocHeader::GetBlock(ptr), allocSize);
            break;
        default:
            NMalloc::AbortFromCorruptedAllocator();
            break;
        }
    }

    const char* GetAllocName(const EAllocType allocType) {
        switch (AllocType) {
            case EAllocType::BAlloc:
                return "b";
            case EAllocType::LF:
                return "lf";
#ifndef _musl_
            case EAllocType::System:
                return "libc";
#endif
            case EAllocType::Mmap:
                return "mmap";
            default:
                return "";
        }
    }

    const char* GetAllocationOwner(void* ptr) {
        return GetAllocName(TAllocHeader::GetAllocType(ptr));
    }

    static bool SetAllocParam(const char* name, const char* value) {
        Init(); //init;
        if (strcmp(name, "disable") == 0) { // for compatibility work as balloc
            if (value == nullptr || strcmp(value, "false") != 0) {
                // all values other than "false" are considred to be "true" for compatibility
                AllocType = DefaultSlaveAlloc;
            } else {
                AllocType = EAllocType::BAlloc;
            }
            return true;
        } else if (strcmp(name, "alloc") == 0) {
            if (strcmp(value, "auto") == 0) {
                AllocType = DefaultAlloc;
            } else if (strcmp(value, "b") == 0) {
                AllocType = EAllocType::BAlloc;
            } else if (strcmp(value, "lf") == 0) {
                AllocType = EAllocType::LF;
            } else if (strcmp(value, "libc") == 0) {
#ifndef _musl_
                AllocType = EAllocType::System;
#else
                return false;
#endif
            } else if (strcmp(value, "mmap") == 0) {
                AllocType = EAllocType::Mmap;
            } else {
                return false;
            }
            return true;
        }

        switch (AllocType) {
#ifndef _darwin_
        case EAllocType::LF:
            return LFAlloc_SetParam(name, value);
#endif
        default:
            break;
        }
        return false;
    }

    static bool CheckAllocParam(const char* name, bool defaultValue) {
        Init();
        if (strcmp(name, "disable") == 0) {
            return AllocType != EAllocType::BAlloc;
        }
        return defaultValue;
    }

    static const char* GetAllocParam(const char* param) {
        Init();
        if (strcmp(param, "disable") == 0) {
            return AllocType == EAllocType::BAlloc ? "false" : "true";
        } else if (strcmp(param, "alloc") == 0) {
            return GetAllocName(AllocType);
        }

        switch (AllocType) {
#ifndef _darwin_
        case EAllocType::LF:
            return LFAlloc_GetParam(param);
#endif
        default:
            break;
        }
        return nullptr;
    }
}  // namespace NCalloc

namespace NMalloc {
    NMalloc::TMallocInfo MallocInfo() {
        NMalloc::TMallocInfo r;
        r.Name = "calloc";
        r.CheckParam = NCalloc::CheckAllocParam;
        r.GetParam = NCalloc::GetAllocParam;
        r.SetParam = NCalloc::SetAllocParam;
        return r;
    }
}  // namespace NMalloc

#if defined(Y_COVER_PTR)
void* CoverPtr(void* ptr, size_t len) noexcept;
void* UncoverPtr(void* ptr) noexcept;
#endif

extern "C" void* malloc(size_t size) {
#if defined(Y_COVER_PTR)
    return CoverPtr(NCalloc::Allocate(size + 32), size);
#else
    return NCalloc::Allocate(size);
#endif
}

extern "C" void free(void* data) {
#if defined(Y_COVER_PTR)
    NCalloc::Free(UncoverPtr(data));
#else
    NCalloc::Free(data);
#endif
}

#if defined(USE_INTELCC) || defined(_darwin_) || defined(_freebsd_) || defined(_STLPORT_VERSION)
#define OP_THROWNOTHING noexcept
#else
#define OP_THROWNOTHING
#endif

void* operator new(size_t size) {
#if defined(Y_COVER_PTR)
    return malloc(size);
#else
    return NCalloc::Allocate(size);
#endif
}

void* operator new(size_t size, const std::nothrow_t&) OP_THROWNOTHING {
#if defined(Y_COVER_PTR)
    return malloc(size);
#else
    return NCalloc::Allocate(size);
#endif
}

void operator delete(void* p)OP_THROWNOTHING {
#if defined(Y_COVER_PTR)
    free(p);
#else
    NCalloc::Free(p);
#endif
}

void operator delete(void* p, const std::nothrow_t&)OP_THROWNOTHING {
#if defined(Y_COVER_PTR)
    free(p);
#else
    NCalloc::Free(p);
#endif
}

void* operator new[](size_t size) {
#if defined(Y_COVER_PTR)
    return malloc(size);
#else
    return NCalloc::Allocate(size);
#endif
}

void* operator new[](size_t size, const std::nothrow_t&) OP_THROWNOTHING{
#if defined(Y_COVER_PTR)
    return malloc(size);
#else
    return NCalloc::Allocate(size);
#endif
}

void operator delete[](void* p) OP_THROWNOTHING{
#if defined(Y_COVER_PTR)
    free(p);
#else
    NCalloc::Free(p);
#endif
}

void operator delete[](void* p, const std::nothrow_t&) OP_THROWNOTHING{
#if defined(Y_COVER_PTR)
    free(p);
#else
    NCalloc::Free(p);
#endif
}

extern "C" void* calloc(size_t n, size_t elemSize) {
    const size_t size = n * elemSize;

    if (elemSize != 0 && size / elemSize != n) {
        return nullptr;
    }

#if defined(Y_COVER_PTR)
    void* result = malloc(size);
#else
    void* result = NCalloc::Allocate(size);
#endif

    if (result) {
        memset(result, 0, size);
    }

    return result;
}

extern "C" void cfree(void* ptr) {
#if defined(Y_COVER_PTR)
    free(ptr);
#else
    NCalloc::Free(ptr);
#endif
}

#if defined(Y_COVER_PTR)
static inline void* DoRealloc(void* oldPtr, size_t newSize) {
#else
extern "C" void* realloc(void* oldPtr, size_t newSize) {
#endif
    if (!oldPtr) {
        void* result = NCalloc::Allocate(newSize);
        return result;
    }
    if (newSize == 0) {
        NCalloc::Free(oldPtr);
        return nullptr;
    }
    void* newPtr = NCalloc::Allocate(newSize);
    if (!newPtr) {
        return nullptr;
    }
    const size_t oldSize = NCalloc::TAllocHeader::GetAllocSize(oldPtr);
    memcpy(newPtr, oldPtr, oldSize < newSize ? oldSize : newSize);
    NCalloc::Free(oldPtr);
    return newPtr;
}

#if defined(Y_COVER_PTR)
extern "C" void* realloc(void* oldPtr, size_t newSize) {
    if (!oldPtr) {
        return malloc(newSize);
    }

    if (!newSize) {
        free(oldPtr);

        return nullptr;
    }

    return CoverPtr(DoRealloc(UncoverPtr(oldPtr), newSize + 32), newSize);
}
#endif

extern "C" int posix_memalign(void** memptr, size_t alignment, size_t size) {
#if defined(Y_COVER_PTR)
    (void)alignment;
    *memptr = malloc(size);
    return 0;
#else
    if (((alignment - 1) & alignment) != 0 || alignment < sizeof(void*)) {
        return EINVAL;
    }
    void* ptr = NCalloc::Allocate(size + alignment);
    *memptr = NCalloc::TAllocHeader::AlignHeader(ptr, alignment);
    return 0;
#endif
}

extern "C" void* memalign(size_t alignment, size_t size) {
    void* ptr;
    int res = posix_memalign(&ptr, alignment, size);
    return res ? nullptr : ptr;
}

#if !defined(_MSC_VER) && !defined(_freebsd_) && !defined(_musl_)
// Workaround for pthread_create bug in linux.
extern "C" void* __libc_memalign(size_t alignment, size_t size) {
    return memalign(alignment, size);
}
#endif
