#pragma once

#include <library/cpp/int128/int128.h>

namespace NDoom::NItemStorage {

using TItemKey = ui128;

class TItemKeyVectorizer {
public:
    enum {
        TupleSize = 4,
    };

    template <class Slice>
    static void Gather(Slice&& slice, TItemKey* data) {
        ui64 hi = (static_cast<ui64>(slice[0]) << 32) | slice[1];
        ui64 lo = (static_cast<ui64>(slice[2]) << 32) | slice[3];

        *data = ui128{hi, lo};
    }

    template <class Slice>
    static void Scatter(TItemKey data, Slice&& slice) {
        ui64 hi = GetHigh(data);
        ui64 lo = GetLow(data);

        slice[0] = hi >> 32;
        slice[1] = hi;
        slice[2] = lo >> 32;
        slice[3] = lo;
    }
};

} // namespace NDoom::NItemStorage
