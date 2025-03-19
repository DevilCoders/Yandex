#pragma once

#include <kernel/snippets/strhl/zonedstring.h>

namespace NSnippets {

    class TSnip;
    class TConfig;

    void GetLinkSpans(const TSnip& snip, TVector<TZonedString::TSpan>& spans);
}
