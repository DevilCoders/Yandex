#pragma once

#include <kernel/lemmer/new_dict/common/new_language.h>

class TEngLanguage: public TNewLanguage {
public:
    NLemmer::TLllLevel LooksLikeLemma(const TYandexLemma& lemma) const override;

    TEngLanguage(ELanguage lang = LANG_ENG)
        : TNewLanguage(lang)
    {
    }

    bool CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2, bool isTranslit) const override;
};
