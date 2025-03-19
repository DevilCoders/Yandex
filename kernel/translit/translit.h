#pragma once

/// author@ vvp@ Victor Ploshikhin
/// created: Nov 14, 2012 6:56:48 PM

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

class TUntransliter;
/**
 * Converts text to unicode and transliterates it symbol by symbol.
 */
void TransliterateBySymbol(const TStringBuf& text, TString& result, const ELanguage textLanguage=LANG_RUS);
void TransliterateBySymbol(const TStringBuf& text, TString& result, TUntransliter* transliterator);
TString TransliterateBySymbol(const TStringBuf& text, TUntransliter* transliterator);
TString TransliterateBySymbol(const TStringBuf& text, const ELanguage textLanguage=LANG_RUS);
