#pragma once

#include <library/cpp/enumbitset/enumbitset.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSymbol {

enum ELemmaQuality {
    LQ_GOOD         /* "good" */,   // Dictionary lemma.
    LQ_BASTARD      /* "bast" */,   // Prefixoids don't have LQ_BASTARD. For prefixoids use LQ_PREFIXOID.
    LQ_FOUND_LING   /* "fling" */,
    LQ_SOB          /* "sob" */,
    LQ_FIX          /* "fix" */,
    LQ_PREFIXOID    /* "pref" */,

    LQ_MAX
};

typedef TEnumBitSet<ELemmaQuality, LQ_GOOD, LQ_MAX> TLemmaQualityBitSet;

TLemmaQualityBitSet ParseLemmaQualities(const TString& str);
TString ToString(const TLemmaQualityBitSet& lemmaQualities);

// Convert lemma quality mask from lemmer to lemma quality type.
ELemmaQuality LemmaQualityFromLemmerQualityMask(ui32 lemmaQualityMask);

} // NSymbol
