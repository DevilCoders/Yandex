#pragma once
#include <util/system/defaults.h>

namespace NMarket::NBalloc {

#if defined(_ppc64_) || defined(_arm64_)
    static constexpr size_t SYSTEM_PAGE_SIZE = 65536;
#else
    static constexpr size_t SYSTEM_PAGE_SIZE = 4096;
#endif


    Y_FORCE_INLINE constexpr size_t AlignUp(size_t value, size_t align) {
        return (value + align - 1) & ~(align - 1);
    }


    Y_FORCE_INLINE void* AlignUp(void* value, size_t align) {
        uintptr_t uiValue = (uintptr_t)value;
        return (void*)((uiValue + align - 1) & ~(align - 1));
    }


    Y_FORCE_INLINE constexpr size_t AlignDown(size_t value, size_t align) {
        return value & ~(align - 1);
    }


    Y_FORCE_INLINE void* AlignDown(void* value, size_t align) {
        uintptr_t uiValue = (uintptr_t)value;
        return (void*)(uiValue & ~(align - 1));
    }
}  // namespace NMarket::NBalloc
