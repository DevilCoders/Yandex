#include "pixel_length.h"
#include "consts.h"

#include <util/generic/utility.h>
#include <util/charset/wide.h>

namespace NSnippets
{
    static const ui16 ARIAL_REGULAR_WIDTH[] = {
        #include "arial_regular.inc"
    };
    static_assert(Y_ARRAY_SIZE(ARIAL_REGULAR_WIDTH) == 65536, "wrong array size");

    static const ui16 ARIAL_BOLD_WIDTH[] = {
        #include "arial_bold.inc"
    };
    static_assert(Y_ARRAY_SIZE(ARIAL_BOLD_WIDTH) == 65536, "wrong array size");

    static const ui32 ARIAL_UNITS_PER_EM = 2048;

    static const wchar16 SPACE_CHAR = wchar16(' ');
    static const wchar16 NON_BREAKING_SPACE_CHAR = wchar16(0xA0);
    static const wchar16 HYPHEN = wchar16('-');
    static const wchar16 FIGURE_DASH = wchar16(0x2012);
    static const wchar16 EN_DASH = wchar16(0x2013);

    static inline bool IsBreakingWhitespace(wchar16 c) {
        // Exotic whitespace characters are not supposed to appear here
        // Non-breaking space is turned into space in basesnip_impl.cpp
        return c == SPACE_CHAR || c == NON_BREAKING_SPACE_CHAR;
    }

    static inline bool CanBreakAfter(wchar16 c, bool isAfterBoldBoundary, bool isBeforeBoldBoundary) {
        // Support only frequent breaking characters that work in all major browsers
        // and can be used in the text far from whitespace characters
        if (c == HYPHEN) {
            // Some browsers don't break the line after a hyphen which follows or precedes a bold text
            return !isAfterBoldBoundary && !isBeforeBoldBoundary;
        }
        return c == FIGURE_DASH || c == EN_DASH;
    }

    static ui32 GetStringLengthInUnits(const TWtringBuf& text, bool isBold) {
        const ui16* width = (isBold ? ARIAL_BOLD_WIDTH : ARIAL_REGULAR_WIDTH);
        ui32 lengthInUnits = 0;
        for (wchar16 c : text) {
            lengthInUnits += width[c];
        }
        return lengthInUnits;
    }

    // NOTE: Multiple successive whitespaces are handled as separate characters
    TPixelLengthCalculator::TPixelLengthCalculator(const TWtringBuf& text, const TVector<TBoldSpan>& boldSpans)
        : BoundaryEllipsisLengthInUnits(GetStringLengthInUnits(BOUNDARY_ELLIPSIS, false))
        , MiddleEllipsisLengthInUnits(GetStringLengthInUnits(MIDDLE_ELLIPSIS, true))
        , AccumLengthInUnits(text.size() + 1, 0)
        , LastFitOffsetToBreakOffset(text.size(), 0)
    {
        size_t boldSpanIndex = 0;
        size_t nextBoldBoundary = boldSpans ? boldSpans[0].BeginOffset : text.size();
        bool isBold = false;
        const ui16* width = ARIAL_REGULAR_WIDTH;
        ui32 sum = 0;
        size_t breakOffset = 0;
        for (size_t i = 0; i < text.size(); ++i) {
            const bool isAfterBoldBoundary = i == nextBoldBoundary;
            while (i >= nextBoldBoundary) {
                if (isBold) {
                    isBold = false;
                    width = ARIAL_REGULAR_WIDTH;
                    if (boldSpanIndex < boldSpans.size()) {
                        nextBoldBoundary = boldSpans[boldSpanIndex].BeginOffset;
                    } else {
                        nextBoldBoundary = text.size();
                    }
                } else {
                    isBold = true;
                    width = ARIAL_BOLD_WIDTH;
                    nextBoldBoundary = boldSpans[boldSpanIndex].EndOffset;
                    ++boldSpanIndex;
                }
            }
            const bool isBeforeBoldBoundary = i + 1 == nextBoldBoundary;
            const wchar16 c = text[i];
            sum += width[c];
            AccumLengthInUnits[i + 1] = sum;
            if (IsBreakingWhitespace(c)) {
                breakOffset = i + 1; // line break after the space is allowed even if the space doesn't fit in the line
            }
            LastFitOffsetToBreakOffset[i] = breakOffset;
            if (CanBreakAfter(c, isAfterBoldBoundary, isBeforeBoldBoundary)) {
                breakOffset = i + 1; // line break after the current character is allowed
            }
        }
    }

    float TPixelLengthCalculator::CalcLengthInPixels(size_t beginOfs, size_t endOfs, float fontSize) const {
        const float unitsPerPixel = ARIAL_UNITS_PER_EM / fontSize;
        return (AccumLengthInUnits[endOfs] - AccumLengthInUnits[beginOfs]) / unitsPerPixel;
    }

    // Line breaking algorithm in word-wrap:break-word mode
    float TPixelLengthCalculator::CalcLengthInRows(size_t beginOfs, size_t endOfs, float fontSize, int pixelsInLine,
        float initialLengthInRows, float prefixLengthInPixels, float suffixLengthInPixels) const
    {
        const float unitsPerPixel = ARIAL_UNITS_PER_EM / fontSize;
        const ui32 unitsInLine = Max(static_cast<ui32>(Max(pixelsInLine, 0) * unitsPerPixel + 1e-4f), 1u);
        const ui32 prefixUnits = static_cast<ui32>(prefixLengthInPixels * unitsPerPixel + 1e-4f);
        const ui32 suffixUnits = static_cast<ui32>(suffixLengthInPixels * unitsPerPixel + 1e-4f);
        const ui32 initialUnits = static_cast<ui32>(initialLengthInRows * unitsInLine + 1e-4f);
        ui32 linesCount = initialUnits / unitsInLine;
        ui32 lastLineUnits = initialUnits % unitsInLine;

        size_t ofs = beginOfs;
        while (ofs < endOfs) {
            ui32 availableUnits = unitsInLine - lastLineUnits;
            if (ofs == beginOfs) {
                availableUnits -= Min(prefixUnits, availableUnits);
            }
            const ui32 maxAccumLength = AccumLengthInUnits[ofs] + availableUnits;
            // Binary search returns overflowOffset:
            // - the first offset after ofs with line overflow: lastLineUnits + [ofs, overflowOffset) >= unitsInLine
            // - or endOfs if the tail fits in the current line
            size_t left = ofs + 1;
            size_t right = endOfs;
            while (left < right) {
                size_t center = (left + right) / 2;
                Y_ASSERT(center < right); // hence center < endOfs, suffix is not added
                if (AccumLengthInUnits[center] < maxAccumLength) { // check if [ofs, center) fits in the current line
                    left = center + 1;
                } else {
                    right = center;
                }
            }
            const size_t overflowOffset = left;
            if (overflowOffset == endOfs) {
                ui32 lastPartUnits = AccumLengthInUnits[endOfs] - AccumLengthInUnits[ofs] +
                    (ofs == beginOfs ? prefixUnits : 0) + suffixUnits;
                if (lastLineUnits + lastPartUnits < unitsInLine) {
                    lastLineUnits += lastPartUnits;
                    break;
                }
            }
            const size_t lastFitOffset = overflowOffset - 1;
            size_t breakOffset = Max(LastFitOffsetToBreakOffset[lastFitOffset], ofs);
            if (breakOffset == ofs && lastLineUnits == 0) { // no break is allowed in the current line
                breakOffset = Max(lastFitOffset, ofs + 1); // break after the last fitting character (but at least one)
            }
            ++linesCount;
            lastLineUnits = 0;
            ofs = breakOffset; // NOTE: breakOffset == ofs is possible in the first iteration
        }

        return static_cast<float>(lastLineUnits) / unitsInLine + linesCount;
    }

    float TPixelLengthCalculator::GetBoundaryEllipsisLength(float fontSize) const {
        const float unitsPerPixel = ARIAL_UNITS_PER_EM / fontSize;
        return BoundaryEllipsisLengthInUnits / unitsPerPixel;
    }

    float TPixelLengthCalculator::GetMiddleEllipsisLength(float fontSize) const {
        const float unitsPerPixel = ARIAL_UNITS_PER_EM / fontSize;
        return MiddleEllipsisLengthInUnits / unitsPerPixel;
    }

    float GetSymbolPixelLength(const wchar16 c, float fontSize) {
        return ARIAL_REGULAR_WIDTH[c] * fontSize / ARIAL_UNITS_PER_EM;
    }

    float GetStringPixelLength(const TWtringBuf& text, float fontSize, bool isBold) {
        const float unitsPerPixel = ARIAL_UNITS_PER_EM / fontSize;
        return GetStringLengthInUnits(text, isBold) / unitsPerPixel;
    }
}
