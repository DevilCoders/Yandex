#pragma once

#include <kernel/lemmer/new_dict/tur/main/tur.h>
#include <kernel/lemmer/new_dict/tur/translit/common/translit.h>

class TTurLanguageWithTrans: public TTurLanguage {
public:
    TAutoPtr<TUntransliter> GetUntransliter(const TUtf16String& word = TUtf16String()) const override {
        return GetTurUntransliter(word);
    }
    TAutoPtr<TUntransliter> GetTransliter(const TUtf16String& word = TUtf16String()) const override {
        return GetTurTransliter(word);
    }
};
