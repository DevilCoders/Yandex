#pragma once

#include <util/generic/yexception.h>

#include <cmath>
#include <limits>
#include <type_traits>

namespace NMath {
    template <class T1, class T2, class = void>
    class TStrict {
    public:
        static T1 Add(const T1 x1, const T2 x2);
    };

    template <class T1, class T2>
    class TStrict<T1, T2,
        std::enable_if_t<
            std::is_integral_v<T1> && std::is_integral_v<T2> &&
            std::is_unsigned_v<T1> && std::is_signed_v<T2> &&
            std::is_same_v<T1, std::make_unsigned_t<T2>>>> {
    public:
        static T1 Add(const T1 x1, const T2 x2) {
            if (x2 < 0) {
                Y_ENSURE(static_cast<T1>(std::abs(x2)) <= x1, "Subtrahend is too big");
            } else {
                Y_ENSURE(static_cast<T1>(x2) <= std::numeric_limits<T1>::max() - x1, "Terms are too big");
            }
            return x1 + x2;
        }
    };

    template <class T1, class T2>
    T1 Add(const T1 x1, const T2 x2) {
        return TStrict<T1, T2>::Add(x1, x2);
    }
}