#pragma once

#include <kernel/lemmer/core/lemmer.h>
#include <library/cpp/langs/langs.h>

namespace NTranslate {
    size_t ToEnglish(const TWtringBuf word, ELanguage language, TWLemmaArray& out, size_t max, const char* needGramm = "");
    size_t FromEnglish(const TWtringBuf word, ELanguage language, TWLemmaArray& out, size_t max, const char* needGramm = "");
};
