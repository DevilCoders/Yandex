#pragma once

#include "single_snip.h"

#include <kernel/snippets/config/enums.h>

#include <kernel/snippets/factors/factor_storage.h>

#include <util/generic/list.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

struct TZonedString;

namespace NSnippets
{
    class TInvalidWeight {
    };
    extern const TInvalidWeight InvalidWeight;

    class TSnip
    {
    public:
        typedef TList<TSingleSnip> TSnips;

    public:
        TSnips Snips;
        double Weight = INVALID_SNIP_WEIGHT;
        TFactorStorage Factors;

    public:
        //empty snippet
        TSnip();
        //snippet with unknown weight and factors
        TSnip(const TSnips& snips, const TInvalidWeight&);
        //snippet with unknown weight and factors
        TSnip(const TSingleSnip& snip, const TInvalidWeight&);
        //many snippets
        TSnip(const TSnips& snips, double weight, TFactorStorage factors);
        //one snippet
        TSnip(const TSingleSnip& snip, double weight, TFactorStorage factors);

        bool HasMatches() const;
        int WordsCount() const;
        TUtf16String GetRawTextWithEllipsis() const;
        TVector<TZonedString> GlueToZonedVec(bool allDots = false, const TSnip& ext = TSnip()) const;
        void GuessAttrs();
        TVector<TString> DumpAttrs() const;
        bool ContainsMetaDescr() const;
        bool RemoveDuplicateSnips();
    };
}
