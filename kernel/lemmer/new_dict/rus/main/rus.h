#pragma once

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/new_dict/common/new_language.h>

class TRusLanguage: public TNewLanguage {
public:
    TRusLanguage(ELanguage lang = LANG_RUS, int version = 1)
        : TNewLanguage(lang, false, version)
    {
    }

protected:
    NLemmer::TLllLevel LooksLikeLemma(const TYandexLemma& lemma) const override;
    TAutoPtr<NLemmer::TGrammarFiltr> LooksLikeLemmaExt(const TYandexLemma& lemma) const override;
    bool CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2, bool isTranslit) const override;

    size_t PostRecognize(TWLemmaArray& lemmas, size_t begin, size_t end, const TConvertResults& cr, const NLemmer::TRecognizeOpt&) const override;

    void TuneOptions(const NLemmer::TAnalyzeWordOpt& awOpt, NLemmer::TRecognizeOpt& rOpt) const override {
        if (!awOpt.DoNotPrune) {
            rOpt.MinAllowedProbability = 0.005;
            rOpt.MaxLemmasToReturn = 4;
        }
    }
};
