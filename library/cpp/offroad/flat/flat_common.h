#pragma once

#include <library/cpp/offroad/utility/masks.h>

#include <util/generic/utility.h>
#include <util/memory/blob.h>
#include <util/system/compiler.h>
#include <util/system/types.h>
#include <util/system/yassert.h>
#include <util/system/unaligned_mem.h>

namespace NOffroad {
    namespace NPrivate {
        enum {
            MaxFastTupleBits = 64 - 8,
            MaxSlowTupleBits = 255,
        };

        [[noreturn]] void ThrowFlatSearcherKeyTooLongException();
        [[noreturn]] void ThrowFlatSearcherDataTooLongException();

        template <size_t Index>
        Y_FORCE_INLINE ui64 SelectBitsFromFlatHeader(const TBlob& header) {
            constexpr ui64 mask = (ui64(1) << 6) - 1;
            constexpr size_t bits = Index * 6;
            constexpr size_t offsetBytes = bits / 8;
            constexpr size_t offsetBits = bits % 8;
            constexpr size_t remaining = 8 - offsetBits;
            Y_ASSERT(offsetBytes < header.Size());
            ui64 result = (header[offsetBytes] >> offsetBits) & mask;
            if constexpr (remaining < 6) {
                constexpr ui64 additionalMask = (ui64(1) << (6 - remaining)) - 1;
                Y_ASSERT(offsetBytes + 1 < header.Size());
                result |= ((header[offsetBytes + 1] & additionalMask) << remaining);
            }
            return result;
        }

        Y_FORCE_INLINE ui64 LoadBits(const TBlob& blob, ui64 offset) {
            ui64 offsetBytes = offset / 8;

            if (Y_LIKELY(offsetBytes + 8 <= blob.Size())) {
                return ReadUnaligned<ui64>(blob.AsCharPtr() + offsetBytes) >> (offset % 8);
            } else {
                ui64 result = 0;
                memcpy(&result, blob.AsCharPtr() + offsetBytes, blob.Size() - offsetBytes);
                return result >> (offset % 8);
            }
        }

        Y_FORCE_INLINE ui64 LoadBits(const TBlob& blob, ui64 offset, ui64 mask) {
            return LoadBits(blob, offset) & mask;
        }

    }
}
