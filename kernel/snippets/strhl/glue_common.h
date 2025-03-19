#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

struct TZonedString;

namespace NSnippets {
    TUtf16String MergedGlue(const TZonedString& z, const TString& attrWithOpenTag = TString(),
        const TString& attrWithCloseTag = TString());

    TVector<TUtf16String> MergedGlue(const TVector<TZonedString>& z);

    TZonedString HtmlEscape(const TZonedString& z, bool skipZeroLenSpans = true);
}

