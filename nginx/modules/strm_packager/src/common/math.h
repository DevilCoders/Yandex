#pragma once

#include <util/system/types.h>

#include <cstdlib>

namespace NStrm::NPackager {
    inline auto ADiv(i64 a, i64 b) {
        auto d = std::div(a, b);
        if (d.rem < 0) {
            if (b > 0) {
                d.rem += b;
                --d.quot;
            } else {
                d.rem -= b;
                ++d.quot;
            }
        }
        return d;
    }

    inline i64 DivFloor(i64 a, i64 b) {
        const auto d = ADiv(a, b);
        return d.quot;
    }

    inline i64 DivCeil(i64 a, i64 b) {
        auto d = ADiv(a, b);
        return d.quot + (d.rem ? 1 : 0);
    }
}
