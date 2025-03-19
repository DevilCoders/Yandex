#include "lemma_quality.h"

#include <kernel/lemmer/core/lemmer.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/vector.h>

namespace NSymbol {

TLemmaQualityBitSet ParseLemmaQualities(const TString& str) {
    TLemmaQualityBitSet lemmaQualities;
    TVector<TString> lemmaQualityNames(SplitString(str.data(), ","));
    for (TVector<TString>::const_iterator it = lemmaQualityNames.begin(); it != lemmaQualityNames.end(); ++it) {
        lemmaQualities.Set(FromString(TStringBuf(it->data(), it->size())));
    }
    return lemmaQualities;
}

TString ToString(const TLemmaQualityBitSet& lemmaQualities) {
    TString str;
    for (ELemmaQuality lq : lemmaQualities) {
        if (!str.empty()) {
            str += ",";
        }
        str += ::ToString(lq);
    }
    return str;
}

ELemmaQuality LemmaQualityFromLemmerQualityMask(ui32 lemmaQualityMask) {
    // Simplifying logic from lemmer-test.
    // Prefixoid check should be first because it's a modifier.
    // Bastard + Prefixoid = LQ_PREFIXOID.
    if (lemmaQualityMask & TYandexLemma::QPrefixoid) {
        return LQ_PREFIXOID;
    }
    if (lemmaQualityMask & TYandexLemma::QBastard) {
        return LQ_BASTARD;
    }
    if (lemmaQualityMask & TYandexLemma::QFoundling) {
        return LQ_FOUND_LING;
    }
    if (lemmaQualityMask & TYandexLemma::QSob) {
        return LQ_SOB;
    }
    if (lemmaQualityMask & TYandexLemma::QFix) {
        return LQ_FIX;
    }
    return LQ_GOOD;
}

} // NSymbol
