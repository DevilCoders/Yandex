#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NSnippets
{
    class TBoldSpan {
    public:
        size_t BeginOffset = 0;
        size_t EndOffset = 0;
    public:
        TBoldSpan(size_t beginOffset, size_t endOffset)
            : BeginOffset(beginOffset)
            , EndOffset(endOffset)
        {
        }
    };

    class TPixelLengthCalculator {
    public:
        TPixelLengthCalculator(const TWtringBuf& text, const TVector<TBoldSpan>& boldSpans);
        float CalcLengthInPixels(size_t beginOfs, size_t endOfs, float fontSize) const;
        float CalcLengthInRows(size_t beginOfs, size_t endOfs, float fontSize, int pixelsInLine,
            float initialLengthInRows = 0.0f, float prefixLengthInPixels = 0.0f, float suffixLengthInPixels = 0.0f) const;
        float GetBoundaryEllipsisLength(float fontSize) const;
        float GetMiddleEllipsisLength(float fontSize) const;
    private:
        const ui32 BoundaryEllipsisLengthInUnits = 0;
        const ui32 MiddleEllipsisLengthInUnits = 0;
        TVector<ui32> AccumLengthInUnits;
        TVector<size_t> LastFitOffsetToBreakOffset;
    };

    float GetSymbolPixelLength(const wchar16 c, float fontSize);
    float GetStringPixelLength(const TWtringBuf& text, float fontSize, bool isBold = false);
}
