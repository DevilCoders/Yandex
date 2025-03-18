#pragma once

#include "impl/tags.h"

#include <contrib/libs/mms/type_traits.h>

namespace NMms {
    template <class T>
    struct TMmappedType {
        typedef typename mms::MmappedType<T>::type Type;
    };

    struct TFastTraits {
        template <class C>
        static inline auto BucketCount(const C& c) {
            size_t r = 1;
            size_t m = (size_t)(c.size() / 0.7);

            while (r < m) {
                r *= 2;
            }

            return r;
        }

        template <class T>
        static inline auto Mod(T a, T b) {
            return a & (b - 1);
        }
    };

}
