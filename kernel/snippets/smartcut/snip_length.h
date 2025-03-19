#pragma once

#include "cutparam.h"

#include <util/generic/vector.h>

namespace NSnippets
{
    class TPixelLengthCalculator;

    class TTextFragment {
    public:
        size_t TextBeginOfs = 0;
        size_t TextEndOfs = 0;
        bool PrependEllipsis = false;
        bool AppendEllipsis = false;
        const TPixelLengthCalculator* Calculator = nullptr;
    };

    class TSnipLengthCalculator {
        TCutParams CutParams;
    public:
        explicit TSnipLengthCalculator(const TCutParams& cutParams);
        float CalcLengthSingle(const TTextFragment& fragment) const;
        float CalcLengthMulti(const TVector<TTextFragment>& fragments) const;
    private:
        float CalcPixelLengthSingle(const TTextFragment& fragment) const;
        float CalcPixelLengthMulti(const TVector<TTextFragment>& fragments) const;
        float CalcSymbolLengthSingle(const TTextFragment& fragment) const;
        float CalcSymbolLengthMulti(const TVector<TTextFragment>& fragments) const;
    };
}
