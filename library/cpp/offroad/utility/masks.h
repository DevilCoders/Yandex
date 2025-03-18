#pragma once

#include <array>

#include <library/cpp/vec4/vec4.h>

namespace NOffroad {
    namespace NPrivate {
        struct TVectorMasks {
            std::array<TVec4u, 33> Values;

            TVectorMasks() {
                for (size_t i = 0; i < 33; ++i) {
                    Values[i] = TVec4u((1ULL << i) - 1ULL);
                }
            }
        };

        struct TScalarMasks {
            std::array<ui64, 65> Values;

            TScalarMasks() {
                for (size_t i = 0; i < 64; ++i) {
                    Values[i] = (1ULL << i) - 1ULL;
                }
                Values[64] = static_cast<ui64>(-1);
            }
        };

        extern TVectorMasks VectorMasks;
        extern TScalarMasks ScalarMasks;

    }

    /**
     * Returns a vector mask that has the provided number of bits set in each element.
     * Basically an equivalent to `TVec4u((1 << bits) - 1)`, but works faster.
     */
    inline const TVec4u& VectorMask(size_t bits) {
        return NPrivate::VectorMasks.Values[bits];
    }

    inline ui64 ScalarMask(size_t bits) {
        return NPrivate::ScalarMasks.Values[bits];
    }

}
