#pragma once

#include <util/system/defaults.h>
#include <util/digest/numeric.h>

namespace NHttpSearchClient {
    typedef ui64 TRequestHash;

    template <class T>
    static inline T ShiftBits(T value) noexcept {
        return IntHash(value);
    }

    template <class T>
    static inline T ShiftBits(T value, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            value = ShiftBits(value);
        }

        return CombineHashes(value, ShiftBits<T>(count));
    }
}
