#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>

namespace NSnippets
{
    class TQueryy;
    class TConfig;
    class TSnip;
    class TSnipTitle;

    class TSimpleSnipCmp : private TNonCopyable
    {
    private:
        TVector<TUtf16String> Txt;
        const TQueryy& Query;
        const TConfig& Cfg;
        const bool SkipStopWords;
        const bool UseWizardWords;

    public:
        TSimpleSnipCmp(const TQueryy& query, const TConfig& cfg, bool skipStopWords = false, bool useWizardWords = false);
        TSimpleSnipCmp& AddUrl(const TString& str);
        TSimpleSnipCmp& Add(const TUtf16String& str);
        TSimpleSnipCmp& Add(const TSnip& snip);
        TSimpleSnipCmp& Add(const TSnipTitle& title);
        bool operator<=(const TSimpleSnipCmp& other) const;
        bool operator>=(const TSimpleSnipCmp& other) const;
        bool operator==(const TSimpleSnipCmp& other) const;
        bool operator<(const TSimpleSnipCmp& other) const;
        bool operator>(const TSimpleSnipCmp& other) const;
        double GetWeight() const;
        bool ContainsAllQueryUserPositions() const;
        static double CalcWeight(const TSnip& snip, const TSnip* titleSnip, bool skipStopWords = false, bool useWizardWords = false);
    };
}
