#pragma once

#include <util/generic/vector.h>
#include <util/generic/list.h>
#include <util/generic/ptr.h>

namespace NSnippets
{
    class TSnipLengthCalculator;
    class TSingleSnip;
    class TSentsMatchInfo;
    class TCutParams;

    class TWordSpanLen
    {
    private:
        const bool IsPixel = false;
        THolder<TSnipLengthCalculator> Calc;

    public:
        TWordSpanLen(const TCutParams& cutParams);
        ~TWordSpanLen();
        float CalcLength(const TSingleSnip& fragment) const;
        float CalcLength(const TVector<TSingleSnip>& fragments) const;
        float CalcLength(const TList<TSingleSnip>& fragments) const;
        int FindFirstWordLonger(const TSentsMatchInfo& matchInfo, int firstWord, int lastWord, float maxLength) const;
    };
}
