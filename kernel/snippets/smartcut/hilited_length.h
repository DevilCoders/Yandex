#pragma once

#include <util/generic/vector.h>

struct TZonedString;

namespace NSnippets
{
    class TBoldSpan;

    void FillBoldSpans(const TZonedString& zoned, TVector<TBoldSpan>& boldSpans);
    float GetHilitedTextLengthInRows(const TVector<TZonedString>& fragments, int pixelsInLine, float fontSize);
    size_t GetHilitedTextLengthInSymbols(const TVector<TZonedString>& fragments);
}
