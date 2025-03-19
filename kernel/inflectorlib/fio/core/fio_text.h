#pragma once

#include "fio_token.h"

#include <kernel/lemmer/dictlib/grammar_enum.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

class TLanguage;

namespace NFioInflector {
    // Example:
    // lang = TNewRussianFIOLanguage::GetLang();
    // query = "persn{Эрих Мария}famn{Ремарк}gender{m}"
    // EGrammar cases[] = { gNominative, gGenitive, gDative, gAccusative, gInstrumental, gAblative, gInvalid };
    TVector<TUtf16String> InflectText(const TLanguage* lang, const TUtf16String& text, const EGrammar cases[], EGrammar gender);
    TVector<TUtf16String> InflectText(const TLanguage* lang, const TUtf16String& text, const EGrammar cases[]);

    EGrammar GuessGender(const TLanguage* lang, const TUtf16String& text);

    template<typename Lang>
    TVector<TUtf16String> InflectText(const TUtf16String& text, const EGrammar cases[], EGrammar gender) {
        const TLanguage* lang = Lang::GetLang();
        return InflectText(lang, text, cases, gender);
    }

    // If text ends on ":m" or ":f", cuts text and returns right gender
    EGrammar CutGender(TUtf16String& text);
}
