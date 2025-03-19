#include "basic_eng.h"

#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>

static const size_t GAP = 16;

static bool Check(const TYandexLemma& lm) {
    using NTGrammarProcessing::HasGram;
    if (lm.GetTextLength() == lm.GetNormalizedFormLength())
        return !memcmp(lm.GetText(), lm.GetNormalizedForm(), lm.GetTextLength() * sizeof(wchar16));
    if (lm.GetNormalizedFormLength() <= 1)
        return false;
    if (lm.GetNormalizedForm()[lm.GetNormalizedFormLength() - 1] != 's')
        return false;
    if (lm.GetTextLength() == lm.GetNormalizedFormLength() - 1 && !memcmp(lm.GetText(), lm.GetNormalizedForm(), lm.GetTextLength() * sizeof(wchar16)))
        return true;
    if (lm.GetNormalizedFormLength() > 2 && lm.GetTextLength() == lm.GetNormalizedFormLength() - 2 && (lm.GetNormalizedForm()[lm.GetNormalizedFormLength() - 2] == '\'' || lm.GetNormalizedForm()[lm.GetNormalizedFormLength() - 2] == 'e')) {
        size_t tocmp = lm.GetNormalizedFormLength() - 2;
        if (tocmp > 1 && lm.GetNormalizedForm()[tocmp] == 'e' && lm.GetNormalizedForm()[tocmp - 1] == 'i' && lm.GetText()[tocmp - 1] == 'y')
            --tocmp;
        if (!memcmp(lm.GetText(), lm.GetNormalizedForm(), tocmp * sizeof(wchar16)))
            return true;
    }

    if (!lm.IsBastard() && (HasGram(lm.GetFlexGram(), lm.FlexGramNum(), gPlural) || HasGram(lm.GetStemGram(), gPlural)))
        return true;
    return false;
}

size_t TBasicEngLanguage::RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const {
    NLemmer::TRecognizeOpt opt2 = opt;
    opt2.MaxLemmasToReturn += GAP;
    return TEngLanguage::RecognizeInternal(word, length, out, opt2);
}

size_t TBasicEngLanguage::PostRecognize(TWLemmaArray& lemmas, size_t begin, size_t end, const TConvertResults&, const NLemmer::TRecognizeOpt&) const {
    for (size_t i = end; i > begin; --i) {
        if (!Check(lemmas[i - 1])) {
            lemmas.erase(lemmas.begin() + (i - 1));
            --end;
        } else {
            NLemmerAux::TYandexLemmaSetter set(lemmas[i - 1]);
            set.SetLanguage(LANG_ENG);
        }
    }
    return end - begin;
}
