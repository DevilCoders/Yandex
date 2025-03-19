#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSnippets {
    class TSnip;
    class TSentsMatchInfo;

    class TEQInfo {
    public:
        TVector<std::pair<TUtf16String, size_t>> SortedWordFrq;
        size_t Total = 0;

    public:
        TEQInfo(const TSnip& snip);
        TEQInfo(const TSentsMatchInfo& smi, int w0, int w1);
        size_t CountEqWords(const TEQInfo& other) const;
    };

    double GetSimilarity(const TEQInfo& a, const TEQInfo& b, bool normOnB = false);
}
