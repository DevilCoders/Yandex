#pragma once

#include "dp_prefix_group_builder.h"
#include "utility.h"

namespace NOffroad {
    namespace NPrivate {
        struct TVec4uTraitsDP {
            static bool ValidPrefix(const TVec4u& vec) {
                return CanFold(vec);
            }
        };

        using TDpPrefixVec4GroupBuilder = TDpPrefixGroupBuilder<TVec4u, 4, 32, TVec4uTraitsDP>;

    }
}
