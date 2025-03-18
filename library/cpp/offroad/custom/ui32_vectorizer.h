#pragma once

#include <util/system/compiler.h>
#include <util/system/types.h>

namespace NOffroad {
    class TUi32Vectorizer {
    public:
        enum {
            TupleSize = 1
        };

        template <class Slice>
        Y_FORCE_INLINE static void Gather(Slice&& slice, ui32* data) {
            *data = slice[0];
        }

        template <class Slice>
        Y_FORCE_INLINE static void Scatter(ui32 data, Slice&& slice) {
            slice[0] = data;
        }
    };

}
