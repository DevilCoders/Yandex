#pragma once

#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/dictlib/grammar_enum.h>
#include <util/generic/string.h>

namespace NInfl {
    TUtf16String Pluralize(
        const TUtf16String& text,
        ui64 targetNumber,
        ELanguage lang = LANG_RUS);

    TUtf16String Pluralize(
        const TUtf16String& text,
        ui64 targetNumber,
        EGrammar targetCase,
        ELanguage lang = LANG_RUS,
        bool isAnimated = false);

    TUtf16String Singularize(
        const TUtf16String& text,
        ui64 sourceNumber,
        ELanguage lang = LANG_RUS);
} // NInfl
