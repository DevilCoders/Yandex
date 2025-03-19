#include "number_format.h"
#include <util/string/cast.h>

namespace NPrivate {
    static const char Prefixes[] {'K', 'M', 'G', 'T'};

    template<int n>
    struct THrNumberHelper {
        static TString GetView(ui64 space) {
            if (space < Limit)
                return THrNumberHelper<n - 1>::GetView(space);
            return ToString(space / Limit).append(' ').append(Prefixes[n - 1]);
        }
        static const ui64 Limit = THrNumberHelper<n - 1>::Limit * 1024;
    };
    template<>
    struct THrNumberHelper<0> {
        static TString GetView(ui64 space) {
            return ToString(space) + " ";
        }
        static const ui64 Limit = 1;
    };
}

TString GetHumanReadableNumber(ui64 number) {
    return NPrivate::THrNumberHelper<Y_ARRAY_SIZE(NPrivate::Prefixes)>::GetView(number);
}
