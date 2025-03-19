#pragma once

#include <kernel/snippets/span/span.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>
#include <utility>

namespace NSnippets
{
    class TSentsMatchInfo;

    class TPLMStatData : private TNonCopyable
    {
    private:
        const TSentsMatchInfo& Info;
        double PosterioriQueryWordWeight;
        TVector<double> QueryWordPosWeight;
        TVector<int> LastQueryWordPositionsInSent;

    public:
        TPLMStatData(const TSentsMatchInfo& info);
        double CalculatePLMScore(const TVector<std::pair<int, int>>& r, bool weightSumOnly = false);
        double CalculateWeightSum(const TSpans& s);
    };
}

