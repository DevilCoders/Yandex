#pragma once

#include <kernel/lemmer/core/language.h>

template <class TLang>
class TLanguageSchwarzCounter {
public:
    TLanguageSchwarzCounter() {
        if (!Counter()++)
            TLang::GetLang();
    }
    ~TLanguageSchwarzCounter() {
        --Counter();
    }

private:
    size_t& Counter() {
        static size_t counter = 0;
        return counter;
    }
};

class TDefaultLanguage: public TLanguage {
public:
    ~TDefaultLanguage() override {
    }
    static const TDefaultLanguage* GetLang() {
        static const TDefaultLanguage TheLang(LANG_UNK);
        return &TheLang;
    }

private:
    TDefaultLanguage(ELanguage id)
        : TLanguage(id)
    {
    }
};

namespace {
    TLanguageSchwarzCounter<TDefaultLanguage> TDefaultLanguageSchwarzCounter;
}
