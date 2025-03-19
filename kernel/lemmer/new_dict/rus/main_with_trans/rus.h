#pragma once

#include <kernel/lemmer/new_dict/rus/main/rus.h>
#include <kernel/lemmer/new_dict/rus/translit/common/translit.h>

class TRusLanguageWithTrans: public TRusLanguage {
public:
    TAutoPtr<TUntransliter> GetUntransliter(const TUtf16String& word = TUtf16String()) const override {
        return GetRusUntransliter(word);
    }
    TAutoPtr<TUntransliter> GetTransliter(const TUtf16String& word = TUtf16String()) const override {
        return GetRusTransliter(word);
    }
    const TTranslationDict* GetUrlTransliteratorDict() const override {
        return NLemmer::NData::GetTranslationRuslit();
    }

    TRusLanguageWithTrans(ELanguage lang = LANG_RUS, int version = 1)
        : TRusLanguage(lang, version)
    {
    }
};
