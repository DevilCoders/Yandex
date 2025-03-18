#include "vowpal_wabbit_predictor.h"

#include <util/generic/bitops.h>
#include <util/string/ascii.h>
#include <util/string/type.h>

namespace NVowpalWabbit {
    namespace MurmurHash3 {
        //-----------------------------------------------------------------------------
        // Block read - if your platform needs to do endian-swapping or can only
        // handle aligned reads, do the conversion here

        static inline ui32 getblock(const ui32* p, int i) {
            return p[i];
        }

        //-----------------------------------------------------------------------------
        // Finalization mix - force all bits of a hash block to avalanche

        static inline ui32 fmix(ui32 h) {
            h ^= h >> 16;
            h *= 0x85ebca6b;
            h ^= h >> 13;
            h *= 0xc2b2ae35;
            h ^= h >> 16;

            return h;
        }
    }

    //-----------------------------------------------------------------------------

    ui32 UniformHash(const void* key, size_t len, ui32 seed) noexcept {
        const ui8* data = reinterpret_cast<const ui8*>(key);
        const int nblocks = static_cast<int>(len / 4);

        ui32 h1 = seed;

        const ui32 c1 = 0xcc9e2d51;
        const ui32 c2 = 0x1b873593;

        // --- body
        const ui32* blocks = (const ui32*)(data + nblocks * 4);

        for (int i = -nblocks; i; i++) {
            ui32 k1 = MurmurHash3::getblock(blocks, i);

            k1 *= c1;
            k1 = RotateBitsLeft(k1, 15);
            k1 *= c2;

            h1 ^= k1;
            h1 = RotateBitsLeft(h1, 13);
            h1 = h1 * 5 + 0xe6546b64;
        }

        // --- tail
        const ui8* tail = (const ui8*)(data + nblocks * 4);

        ui32 k1 = 0;

        switch (len & 3) {
            case 3:
                k1 ^= tail[2] << 16;
                [[fallthrough]];
            case 2:
                k1 ^= tail[1] << 8;
                [[fallthrough]];
            case 1:
                k1 ^= tail[0];
                k1 *= c1;
                k1 = RotateBitsLeft(k1, 15);
                k1 *= c2;
                h1 ^= k1;
                break;
        }

        // --- finalization
        h1 ^= len;

        return MurmurHash3::fmix(h1);
    }

    ui32 HashString(const char* begin, const char* end, ui32 seed) noexcept {
        ui32 ret = 0;
        const char* p = begin;

        while (p != end) {
            if (IsAsciiDigit(*p)) {
                ret = 10 * ret + *(p++) - '0';
            } else {
                return UniformHash(begin, end - begin, seed);
            }
        }

        return ret + seed;
    }
}
