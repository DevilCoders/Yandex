#pragma once

#include "kaz_detranslit.h"

#include <kernel/lemmer/new_dict/kaz/main/kaz.h>
#include <kernel/lemmer/new_dict/kaz/translit/common/translit.h>

class TKazLanguageWithTrans: public TKazLanguage {
public:
    TAutoPtr<TUntransliter> GetUntransliter(const TUtf16String& word = TUtf16String()) const override {
        return GetKazUntransliter(word);
    }
    TAutoPtr<TUntransliter> GetTransliter(const TUtf16String& word = TUtf16String()) const override {
        return GetKazTransliter(word);
    }
    TAutoPtr<IDetransliterator> GetDetransliterator() const override {
        return GetKazDetransliterator();
    }
};
