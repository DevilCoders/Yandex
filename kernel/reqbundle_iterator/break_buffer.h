#pragma once

#include <util/system/types.h>

#include <array>

namespace NReqBundleIteratorImpl {
    class TBreakBuffer {
    public:
        enum {
           BreakCount = 1 << 15,
        };

        std::array<ui8, BreakCount> Buffer;
        std::array<ui16, BreakCount> Ptr;
        size_t Index = 0;

    public:
        TBreakBuffer() {
            Buffer.fill(1);
        }
        void Clear() {
            for (size_t i = 0; i < Index; ++i) {
                Buffer[Ptr[i]] = 1;
            }
            Index = 0;
        }
        void SetBrk(ui16 brk) {
            Ptr[Index] = brk;
            Index += Buffer[brk];
            Buffer[brk] = 0;
        }
        bool EmptyBrk(ui16 brk) const {
            return Buffer[brk] == 1;
        }
    };
} // NReqBundleIteratorImpl
