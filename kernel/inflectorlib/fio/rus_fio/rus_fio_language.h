#pragma once

#include <kernel/lemmer/new_dict/common/new_language.h>

class TRusFIOLanguage: public TNewLanguage {
public:
    static const TRusFIOLanguage* GetLang();
    TRusFIOLanguage();
};

