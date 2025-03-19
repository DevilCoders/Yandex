#pragma once

#include <kernel/lemmer/new_dict/ukr/main/ukr.h>
#include <kernel/lemmer/new_dict/ukr/translit/common/translit.h>

class TUkrLanguageWithTrans: public TUkrLanguage {
public:
    TAutoPtr<TUntransliter> GetUntransliter(const TUtf16String& word = TUtf16String()) const override {
        return GetUkrUntransliter(word);
    }
    TAutoPtr<TUntransliter> GetTransliter(const TUtf16String& word = TUtf16String()) const override {
        return GetUkrTransliter(word);
    }
    const TTranslationDict* GetUrlTransliteratorDict() const override {
        return NLemmer::NData::GetTranslationUkrlit();
    }
};
