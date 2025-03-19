#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

namespace NSnippets
{
    bool HasTooManyWordsOfAlphabet(const TUtf16String& text, ELanguage lang, int wordsThreshold);
    bool HasWordsOfAlphabet(const TUtf16String& text, ELanguage lang);
    bool HasTooManyCyrillicWords(const TUtf16String& text, int cyrWordThreshold);
    double MeasureFractionOfLang(const TUtf16String& text, ELanguage lang);
}
