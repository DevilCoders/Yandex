#pragma once

#include <kernel/lemmer/new_dict/common/new_language.h>

class TUkrLanguage: public TLanguageTemplate<LANG_UKR> {
public:
    NLemmer::TLllLevel LooksLikeLemma(const TYandexLemma& lemma) const override;
    TAutoPtr<NLemmer::TGrammarFiltr> LooksLikeLemmaExt(const TYandexLemma& lemma) const override;

private:
    bool CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2, bool isTranslit) const override;
};
