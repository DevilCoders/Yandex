#pragma once

#include "greedy_prefix_group_builder.h"
#include "utility.h"

namespace NOffroad {
    namespace NPrivate {
        struct TVec4uTraits {
            static bool ValidPrefix(const TVec4u& vec) {
                return CanFold(vec);
            }
        };

        using TGreedyPrefixVec4GroupBuilder = TGreedyPrefixGroupBuilder<TVec4u, 4, 32, TVec4uTraits>;

    }
}
