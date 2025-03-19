#pragma once

#include "tokens.h"

#include <library/cpp/langs/langs.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <util/generic/string.h>

namespace NUrlCutter {
    class TRichTreeWanderer;

    TTokenList TokenizeAndHilite(const TUtf16String& url, TRichTreeWanderer& rTreeWanderer,
        ELanguage lang, EPathType ptype);
}
