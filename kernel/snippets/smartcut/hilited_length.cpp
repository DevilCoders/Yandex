#include "hilited_length.h"
#include "pixel_length.h"
#include "consts.h"

#include <kernel/snippets/strhl/hilite_mark.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <util/generic/string.h>

namespace NSnippets
{
    static bool IsHilitedMark(const THiliteMark* mark) {
        if (!mark) {
            return false;
        }
        if (mark == &DEFAULT_MARKS) {
            return true;
        }
        return mark->OpenTag == DEFAULT_MARKS.OpenTag &&
               mark->CloseTag == DEFAULT_MARKS.CloseTag;
    }

    void FillBoldSpans(const TZonedString& zoned, TVector<TBoldSpan>& boldSpans) {
        for (const auto& entry : zoned.Zones) {
            if (IsHilitedMark(entry.second.Mark)) {
                for (const auto& span : entry.second.Spans) {
                    boldSpans.push_back(TBoldSpan(~span - zoned.String.data(), ~span + +span - zoned.String.data()));
                }
            }
        }
    }

    float GetHilitedTextLengthInRows(const TVector<TZonedString>& fragments, int pixelsInLine, float fontSize)
    {
        float lengthInRows = 0.0f;
        for (size_t i = 0; i < fragments.size(); ++i) {
            float suffixLengthInPixels = 0.0f;
            // Between fragments: space, bold ellipsis, space
            if (i + 1 < fragments.size()) {
                suffixLengthInPixels += GetStringPixelLength(MIDDLE_ELLIPSIS, fontSize, true);
            }
            const TUtf16String& plainText = fragments[i].String;
            TVector<TBoldSpan> boldSpans;
            FillBoldSpans(fragments[i], boldSpans);
            TPixelLengthCalculator calculator(plainText, boldSpans);
            lengthInRows = calculator.CalcLengthInRows(0, plainText.size(), fontSize, pixelsInLine, lengthInRows, 0.0f, suffixLengthInPixels);
        }
        return lengthInRows;
    }

    size_t GetHilitedTextLengthInSymbols(const TVector<TZonedString>& fragments)
    {
        size_t symbols = 0;
        for (size_t i = 0; i < fragments.size(); ++i) {
            symbols += fragments[i].String.length();
            // Between fragments: space, ellipsis, space
            if (i + 1 < fragments.size()) {
                symbols += MIDDLE_ELLIPSIS.size();
            }
        }
        return symbols;
    }
}
