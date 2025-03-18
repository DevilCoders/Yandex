#include <library/cpp/balloc_market/lib/balloc.h>
#include <errno.h>

namespace NMarket::NBalloc {

    static constexpr size_t ALIVE_SIGNATURE = 0xaULL << 56;
    static constexpr size_t DISABLED_SIGNATURE = 0xbULL << 56;
    static constexpr size_t SIGNATURE_MASK = 0xfULL << 56;

    static constexpr size_t MINIMAL_ALIGNMENT = NCalloc::ALLOC_HEADER_ALIGNMENT;
    static_assert(((MINIMAL_ALIGNMENT - 1) & MINIMAL_ALIGNMENT) == 0,
        "invalid BALLOC_MINIMAL_ALIGNMENT");


    static Y_FORCE_INLINE void* MallocBallocForeverAndOnly(size_t size) {
        TLS& ltls = tls;
        if (Y_LIKELY(ltls.Mode == Alive))
            return AllocateRawOnly(size);
        if (Y_UNLIKELY(ltls.Mode == Empty || ltls.Mode == ToBeEnabled)) {
            Init();
        }
        if (Y_UNLIKELY(ltls.Mode == Disabled))
            NMalloc::AbortFromCorruptedAllocator();
        return AllocateRawOnly(size);
    }


    static Y_FORCE_INLINE void* Malloc(size_t size) {
        if (NAllocSetup::IsBallocForeverAndOnly())
            return MallocBallocForeverAndOnly(size);

        TLS& ltls = tls;
        if (Y_LIKELY(ltls.Mode == Alive)) {
            TAllocHeader* allocHeader;
            allocHeader = AllocateRaw(size, ALIVE_SIGNATURE);
            return allocHeader + 1;
        }
        if (Y_UNLIKELY(ltls.Mode == Empty || ltls.Mode == ToBeEnabled)) {
            Init();
        }
        TAllocHeader* allocHeader;
        if (Y_LIKELY(ltls.Mode != Disabled)) {
            allocHeader = AllocateRaw(size, ALIVE_SIGNATURE);
        } else {
            // ltls.Mode == Disabled
            size = AlignUp(size, NCalloc::ALLOC_HEADER_ALIGNMENT);
            const size_t extsize = size + sizeof(TAllocHeader);
            allocHeader = (TAllocHeader*)LibcMalloc(extsize);
            allocHeader->Encode(allocHeader, size, DISABLED_SIGNATURE);
            // see ABOUT STORE AND LOAD FENCES in balloc.h
            std::atomic_thread_fence(std::memory_order_release);
        }
        return allocHeader + 1;
    }


    static void Y_FORCE_INLINE Free(void* ptr) {
        if (Y_UNLIKELY(ptr == nullptr))
            return;

        if (NAllocSetup::IsBallocForeverAndOnly()) {
            FreeRaw(AlignDown(ptr, SYSTEM_PAGE_SIZE));
            return;
        }

        // see ABOUT STORE AND LOAD FENCES in balloc.h
        std::atomic_thread_fence(std::memory_order_acquire);

        TAllocHeader* allocHeader = ((TAllocHeader*)ptr) - 1;
        size_t size = allocHeader->AllocSize;
        const size_t signature = size & SIGNATURE_MASK;
        if (Y_LIKELY(signature == ALIVE_SIGNATURE)) {
            allocHeader->AllocSize = 0; // abort later on double free
            ConditionalFillMemory(ptr, 0xde, size - signature);
            FreeRaw(allocHeader->Block);
            if (NAllocStats::IsEnabled()) {
                NAllocStats::DecThreadAllocStats(size - signature);
            }
        } else if (signature == DISABLED_SIGNATURE) {
            LibcFree(allocHeader->Block);
        } else {
            NMalloc::AbortFromCorruptedAllocator();
        }
    }


    static bool Y_FORCE_INLINE IsOwnedByBalloc(void* ptr) {
        if (NAllocSetup::IsBallocForeverAndOnly())
            return true;

        // see ABOUT STORE AND LOAD FENCES in balloc.h
        std::atomic_thread_fence(std::memory_order_acquire);

        TAllocHeader* allocHeader = ((TAllocHeader*)ptr) - 1;
        size_t size = allocHeader->AllocSize;
        const size_t signature = size & SIGNATURE_MASK;
        if (signature == ALIVE_SIGNATURE) {
            return true;
        } else if (signature == DISABLED_SIGNATURE) {
            return false;
        }
        NMalloc::AbortFromCorruptedAllocator();
        Y_UNREACHABLE();
    }


    static void Y_FORCE_INLINE Disable() {
#    if defined(_musl_)
    // just skip it
#    else
        if (NAllocSetup::IsBallocForeverAndOnly())
            NMalloc::AbortFromCorruptedAllocator();
        tls.Mode = Disabled;
#    endif
    }


    static void Y_FORCE_INLINE Enable() {
        if (tls.Mode != Alive)
            tls.Mode = ToBeEnabled;
    }

    static bool Y_FORCE_INLINE IsDisabled() {
        return tls.Mode == Disabled;
    }
}  // namespace NMarket::NBalloc

using namespace NMarket;

#if defined(Y_COVER_PTR)
void* CoverPtr(void* ptr, size_t len) noexcept;
void* UncoverPtr(void* ptr) noexcept;
#endif

extern "C" void* malloc(size_t size) {
#if defined(Y_COVER_PTR)
    return CoverPtr(NBalloc::Malloc(size + 32), size);
#else
    return NBalloc::Malloc(size);
#endif
}

extern "C" void free(void* data) {
#if defined(Y_COVER_PTR)
    NBalloc::Free(UncoverPtr(data));
#else
    NBalloc::Free(data);
#endif
}

void* operator new(size_t size) {
#if defined(Y_COVER_PTR)
    return malloc(size);
#else
    return NBalloc::Malloc(size);
#endif
}

int posix_memalign(void** memptr, const size_t alignment, const size_t size) {
#if defined(Y_COVER_PTR)
    (void)alignment;
    *memptr = malloc(size);
    return 0;
#else
    if (((alignment - 1) & alignment) != 0 || alignment < sizeof(void*)) {
        return EINVAL;
    }
    if (alignment <= NBalloc::MINIMAL_ALIGNMENT) {
        *memptr = NBalloc::Malloc(size);
        return 0;
    }
    size_t bigSize = size + alignment - NBalloc::MINIMAL_ALIGNMENT;
    void* res = NBalloc::Malloc(bigSize);
    void* alignedPtr = (void*)NBalloc::AlignUp((size_t)res, alignment);

    if (!NAllocSetup::IsBallocForeverAndOnly() && alignedPtr != res) {
        auto oldAllocHeader = (NBalloc::TAllocHeader*)res - 1;
        auto newAllocHeader = (NBalloc::TAllocHeader*)alignedPtr - 1;
        void* block = oldAllocHeader->Block;
        newAllocHeader->Encode(block, size, NBalloc::ALIVE_SIGNATURE);
        // see ABOUT STORE AND LOAD FENCES in balloc.h
        std::atomic_thread_fence(std::memory_order_release);
    }

    *memptr = alignedPtr;
    return 0;
#endif
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
#if defined(Y_COVER_PTR)
    return malloc(size);
#else
    return NBalloc::Malloc(size);
#endif
}

void operator delete(void* p) noexcept {
#if defined(Y_COVER_PTR)
    free(p);
#else
    NBalloc::Free(p);
#endif
}

void operator delete(void* p, const std::nothrow_t&) noexcept {
#if defined(Y_COVER_PTR)
    free(p);
#else
    NBalloc::Free(p);
#endif
}

void* operator new[](size_t size) {
#if defined(Y_COVER_PTR)
    return malloc(size);
#else
    return NBalloc::Malloc(size);
#endif
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
#if defined(Y_COVER_PTR)
    return malloc(size);
#else
    return NBalloc::Malloc(size);
#endif
}

void operator delete[](void* p) noexcept {
#if defined(Y_COVER_PTR)
    free(p);
#else
    NBalloc::Free(p);
#endif
}

void operator delete[](void* p, const std::nothrow_t&) noexcept {
#if defined(Y_COVER_PTR)
    free(p);
#else
    NBalloc::Free(p);
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
    void* result = NBalloc::Malloc(size);
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
    NBalloc::Free(ptr);
#endif
}

#if defined(Y_COVER_PTR)
static inline void* DoRealloc(void* oldPtr, size_t newSize) {
#else
extern "C" void* realloc(void* oldPtr, size_t newSize) {
#endif
    if (!oldPtr) {
        void* result = NBalloc::Malloc(newSize);
        return result;
    }
    if (newSize == 0) {
        NBalloc::Free(oldPtr);
        return nullptr;
    }
    void* newPtr = NBalloc::Malloc(newSize);
    if (!newPtr) {
        return nullptr;
    }

    if (NAllocSetup::IsBallocForeverAndOnly()) {
        std::atomic_thread_fence(std::memory_order_acquire);
        auto block = (NBalloc::TBlockHeader*)NBalloc::AlignDown(
            oldPtr, NBalloc::SYSTEM_PAGE_SIZE);
        size_t maxValidSize =
            block->Size - ((size_t)oldPtr & (NBalloc::SYSTEM_PAGE_SIZE - 1));
        if (newSize < maxValidSize)
            maxValidSize = newSize;
        memcpy(newPtr, oldPtr, maxValidSize);
        NBalloc::Free(oldPtr);
        std::atomic_thread_fence(std::memory_order_release);
        return newPtr;
    }

    NBalloc::TAllocHeader* header = (NBalloc::TAllocHeader*)oldPtr - 1;
    // see ABOUT STORE AND LOAD FENCES in balloc.h
    std::atomic_thread_fence(std::memory_order_acquire);
    const size_t oldSize = header->AllocSize & ~NBalloc::SIGNATURE_MASK;
    const size_t signature = header->AllocSize & NBalloc::SIGNATURE_MASK;
    if (Y_LIKELY((signature == NBalloc::ALIVE_SIGNATURE) || (signature == NBalloc::DISABLED_SIGNATURE))) {
        memcpy(newPtr, oldPtr, oldSize < newSize ? oldSize : newSize);
        NBalloc::Free(oldPtr);
        return newPtr;
    }
    NMalloc::AbortFromCorruptedAllocator();
    return nullptr;
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

// Only for testing purposes. Never use in production.
extern "C" bool IsOwnedByBalloc(void* ptr) {
    return NBalloc::IsOwnedByBalloc(ptr);
}

extern "C" bool BallocDisabled() {
    return NBalloc::IsDisabled();
}

extern "C" void DisableBalloc() {
    NBalloc::Disable();
}

extern "C" void EnableBalloc() {
    NBalloc::Enable();
}

extern "C" void* memalign(size_t alignment, size_t size) {
    void* ptr;
    int res = posix_memalign(&ptr, alignment, size);
    return res ? nullptr : ptr;
}

#if !defined(_MSC_VER) && !defined(_freebsd_)
// Workaround for pthread_create bug in linux.
extern "C" void* __libc_memalign(size_t alignment, size_t size) {
    return memalign(alignment, size);
}
#endif
