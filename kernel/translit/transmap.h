#pragma once

#include "symbolmaps.h"

#include <library/cpp/langs/langs.h>

//TODO add hebrew and other languages.
//To get the table, search: "romanization of X"

namespace NTranslit {
    void TableTranslitBySymbol(const TWtringBuf& src, TString& dst, const TCharTable& charTable);
}

void TableTranslitBySymbol(const TWtringBuf& src, ELanguage lang, TString& dst);
