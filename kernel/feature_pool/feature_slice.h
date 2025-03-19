#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NMLPool {
    struct TFeatureSlice {
        TString Name;
        size_t Begin = 0;
        size_t End = 0;

        TFeatureSlice() = default;
        TFeatureSlice(const TString& name,
                      size_t begin, size_t end)
            : Name(name)
            , Begin(begin)
            , End(end)
        {
        }
    };
    using TFeatureSlices = TVector<TFeatureSlice>;
}
