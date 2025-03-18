#pragma once

#include <util/system/compiler.h>
#include <util/system/types.h>

namespace NOffroad {
    class TUi64Vectorizer {
    public:
        enum {
            TupleSize = 2
        };

        template <class Slice>
        Y_FORCE_INLINE static void Gather(Slice&& slice, ui64* data) {
            *data = (static_cast<ui64>(slice[0]) << 32) | slice[1];
        }

        template <class Slice>
        Y_FORCE_INLINE static void Scatter(ui64 data, Slice&& slice) {
            slice[0] = data >> 32;
            slice[1] = data;
        }
    };

}
