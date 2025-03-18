#pragma once

#include <util/system/compiler.h>
#include <util/system/types.h>
#include <util/stream/output.h>

namespace NOffroad {
    template <class Unsigned>
    struct TVarintSerializer {
        static_assert(std::is_unsigned<Unsigned>::value, "Expecting an unsigned type here.");

        enum {
            MaxSize = (std::numeric_limits<Unsigned>::digits + 6) / 7
        };

        Y_FORCE_INLINE static size_t Serialize(Unsigned value, ui8* dst) {
            size_t size = 1;
            while (true) {
                if (value < 128) {
                    *dst++ = value;
                    return size;
                } else {
                    *dst++ = (value & 127) + 128;
                    value = (value >> 7);
                    size++;
                }
            }
        }

        Y_FORCE_INLINE static size_t Deserialize(const ui8* src, Unsigned* value) {
            size_t shift = 0;
            Unsigned result = 0;
            while (true) {
                Unsigned byte = *src++;
                result |= ((byte & 127) << shift);
                if (byte < 128) {
                    *value = result;
                    return shift / 7 + 1;
                }
                shift += 7;
            }
        }
    };

}
