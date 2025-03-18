#pragma once

#include <util/system/yassert.h>

#include <library/cpp/offroad/utility/masks.h>
#include <library/cpp/vec4/vec4.h>

#include <type_traits> /* For std::is_unsigned. */

namespace NOffroad {
    namespace NPrivate {
        // MMIX by Donald Knuth
        static const ui64 PseudoRandomMultiplier = 6364136223846793005ULL;
        static const ui64 PseudoRandomAdder = 1442695040888963407ULL;

        static const ui64 Mask7fff = 0x7fff7fff7fff7fffULL;

        inline ui64 NextPseudoRandom(ui64 value) {
            return value * PseudoRandomMultiplier + PseudoRandomAdder;
        }

        /**
         * Checks whether all elements of the provided vector are less than `2^15`, and
         * thus the vector can be packed into an `ui64` via `ToUI64`.
         */
        inline bool CanFold(const TVec4u& vector) {
            return vector.HasMask(VectorMask(15));
        }

        /**
         * Packs provided vector into an `ui64`. It's up to the user to check that all
         * values in the vector are actually less than `2^15`.
         *
         * Note that the `2^15` requirement comes from the fact that `_mm_packs_epi32`
         * operates on signed integers with saturation.
         *
         * @see CanFold
         */
        inline ui64 ToUI64(const TVec4u& vector) {
            Y_ASSERT(CanFold(vector));

            return _mm_cvtsi128_si64(_mm_packs_epi32(vector.V128(), vector.V128()));
        }

        /**
         * Unpacks provided `ui64` value into a vector.
         */
        inline TVec4u FromUI64(const ui64& value) {
            return static_cast<__m128i>(_mm_unpacklo_epi16(_mm_cvtsi64_si128(value), _mm_setzero_si128()));
        }

        template <class Unsigned>
        inline void AssertSupportedType() {
            static_assert(sizeof(Unsigned) <= sizeof(ui32), "Can't handle unsigned types larger than 32 bits.");
            static_assert(std::is_unsigned<Unsigned>::value, "Expecting an unsigned type here.");
        }

        template <class Unsigned>
        inline TVec4u LoadVec(const Unsigned* src) {
            if (sizeof(Unsigned) == sizeof(ui32)) {
                return TVec4u(reinterpret_cast<const ui32*>(src));
            } else {
                return TVec4u(src[0], src[1], src[2], src[3]);
            }
        }

        template <class Unsigned>
        inline void StoreVec(const TVec4u& src, Unsigned* dst) {
            if (sizeof(Unsigned) == sizeof(ui32)) {
                src.Store(reinterpret_cast<ui32*>(dst));
            } else {
                ui32 tmp[4];
                src.Store(tmp);

                for (size_t i = 0; i < 4; i++)
                    dst[i] = tmp[i];
            }
        }

        template <class Unsigned>
        inline void StoreVec4(const TVec4u& a, const TVec4u& b, const TVec4u& c, const TVec4u& d, Unsigned* dst) {
            if (sizeof(Unsigned) == sizeof(ui8)) {
                Pack(a, b, c, d).Store(dst);
            } else {
                StoreVec(a, dst + 0);
                StoreVec(b, dst + 4);
                StoreVec(c, dst + 8);
                StoreVec(d, dst + 12);
            }
        }

    }
}
