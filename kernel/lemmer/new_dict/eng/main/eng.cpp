#include "eng.h"

#include <kernel/lemmer/core/rubbishdump.h>

using NTGrammarProcessing::HasGram;

static NLemmer::TLllLevel CheckLLLA(const char* stemGram, const char* const flexGram[], size_t gramNum) {
    if (!HasGram(stemGram, gAdjective) && !HasGram(stemGram, gAdverb))
        return NLemmer::LllNo;
    if (HasGram(flexGram, gramNum, gComparative) || HasGram(flexGram, gramNum, gSuperlative))
        return NLemmer::LllNo;
    return NLemmer::Lll;
}

static NLemmer::TLllLevel CheckLLLS(const char* stemGram, const char* const flexGram[], size_t gramNum) {
    if (!HasGram(stemGram, gSubstantive) || !HasGram(flexGram, gramNum, gNominative))
        return NLemmer::LllNo;
    //    if (HasGram(stemGram, gSingular) || HasGram(stemGram, gPlural))
    //        return NLemmer::LllTantum;
    return NLemmer::Lll;
}

static NLemmer::TLllLevel CheckLLLV(const char* stemGram, const char* const flexGram[], size_t gramNum) {
    if (!HasGram(stemGram, gVerb) || !HasGram(flexGram, gramNum, gInfinitive))
        return NLemmer::LllNo;
    return NLemmer::Lll;
}

NLemmer::TLllLevel TEngLanguage::LooksLikeLemma(const TYandexLemma& lemma) const {
    NLemmer::TLllLevel r = NLemmer::LllNo;

    NLemmer::TLllLevel rt = NLanguageRubbishDump::DefaultLooksLikeLemma(lemma);
    if (rt > r)
        r = rt;

    rt = CheckLLLA(lemma.GetStemGram(), lemma.GetFlexGram(), lemma.FlexGramNum());
    if (rt > r)
        r = rt;

    rt = CheckLLLS(lemma.GetStemGram(), lemma.GetFlexGram(), lemma.FlexGramNum());
    if (rt > r)
        r = rt;

    rt = CheckLLLV(lemma.GetStemGram(), lemma.GetFlexGram(), lemma.FlexGramNum());
    if (rt > r)
        r = rt;

    return r;
}

bool TEngLanguage::CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2, bool /*isTranslit*/) const {
    return NLanguageRubbishDump::EnglishCanBreak(word, pos1, len1, pos2, len2);
}
