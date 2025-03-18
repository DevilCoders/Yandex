#pragma once

#include <util/system/compiler.h>

namespace NScores {
    namespace NDetail {
        template <typename Result, typename T, typename U>
        inline Result DivideOrZero(const T numerator, const U denominator) {
            if (Y_UNLIKELY(0 == denominator))
                return Result();

            return static_cast<Result>(numerator) / denominator;
        }

    }

}
