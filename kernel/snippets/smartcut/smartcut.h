#pragma once

// https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/annotation/smartcut/

#include "multi_length_cut.h"
#include "pixel_length.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>

class TWordFilter;
class TInlineHighlighter;

namespace NSnippets
{
    class TCutParams;
    class TBoldSpan;

    class TTextCuttingOptions {
    public:
        const TWordFilter* StopWordsFilter = nullptr;
        bool MaximizeLen = false;
        float Threshold = 2.0f / 3.0f;
        bool AddEllipsisToShortText = false;
        bool CutLastWord = false;
        bool AddBoundaryEllipsis = true;
        const wchar16* HilightMark = nullptr;
    };

    TUtf16String SmartCutSymbol(
        const TUtf16String& source,
        size_t maxSymbols,
        const TTextCuttingOptions& options = TTextCuttingOptions());

    TUtf16String SmartCutPixelNoQuery(
        const TUtf16String& source,
        float maxRows,
        int pixelsInLine,
        float fontSize,
        const TTextCuttingOptions& options = TTextCuttingOptions(),
        const TVector<TBoldSpan>& boldSpans = TVector<TBoldSpan>());

    TUtf16String SmartCutPixelWithQuery(
        const TUtf16String& source,
        const TInlineHighlighter& highlighter,
        float maxRows,
        int pixelsInLine,
        float fontSize,
        const TTextCuttingOptions& options = TTextCuttingOptions());


    TUtf16String SmartCutPixelCustomWithQuery(
        const TUtf16String& source,
        const TPixelLengthCalculator& calc,
        float maxRows,
        int pixelsInLine,
        float fontSize,
        const TTextCuttingOptions& options = TTextCuttingOptions());

    TUtf16String SmartCutWithQuery(
        const TUtf16String& source,
        const TInlineHighlighter& highlighter,
        float maxLength,
        const TCutParams& cutParams,
        const TTextCuttingOptions& options = TTextCuttingOptions());

    TMultiCutResult SmartCutExtSnipWithQuery(
        const TUtf16String& source,
        const TInlineHighlighter& highlighter,
        float maxLengthShort,
        float maxLengthLong,
        const TCutParams& cutParams,
        const TTextCuttingOptions& options = TTextCuttingOptions());
}
