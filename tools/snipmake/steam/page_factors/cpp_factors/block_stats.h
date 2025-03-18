#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSegmentator {

class TTextStats {
public:
    TTextStats(const TUtf16String& text);

    void CalcTextStats();
    void Update(const TTextStats& stats);

public:
    const TUtf16String Text;
    size_t AlphasCount = 0;
    size_t UpperAlphasCount = 0;
    size_t DigitsCount = 0;
    size_t PunctsCount = 0;
    size_t MidPunctsCount = 0;
    size_t EndPunctsCount = 0;

    size_t WordsCount = 0;
};


class TBlockStats {
public:
    TBlockStats(const TWtringBuf& block);

private:
    static TUtf16String CollapseText(const TWtringBuf& text);
    void CalcStats();

public:
    TTextStats BlockStats;
    TVector<TTextStats> ParsStats;
    TVector<TTextStats> SentsStats;

    TVector<size_t> SentsInParsCounts;
};

}  // NSegmentator
