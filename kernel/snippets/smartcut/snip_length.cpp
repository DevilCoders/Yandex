#include "snip_length.h"
#include "pixel_length.h"
#include "consts.h"

#include <util/charset/wide.h>
#include <util/generic/string.h>

namespace NSnippets
{
    TSnipLengthCalculator::TSnipLengthCalculator(const TCutParams& cutParams)
        : CutParams(cutParams)
    {
    }

    float TSnipLengthCalculator::CalcLengthSingle(const TTextFragment& fragment) const {
        if (CutParams.IsPixel) {
            return CalcPixelLengthSingle(fragment);
        } else {
            return CalcSymbolLengthSingle(fragment);
        }
    }

    float TSnipLengthCalculator::CalcLengthMulti(const TVector<TTextFragment>& fragments) const {
        if (CutParams.IsPixel) {
            return CalcPixelLengthMulti(fragments);
        } else {
            return CalcSymbolLengthMulti(fragments);
        }
    }

    float TSnipLengthCalculator::CalcPixelLengthSingle(const TTextFragment& fragment) const {
        Y_ASSERT(fragment.Calculator);
        float prefixLengthInPixels = 0.0f;
        float suffixLengthInPixels = 0.0f;
        // Before the first fragment
        if (fragment.PrependEllipsis) {
            prefixLengthInPixels += fragment.Calculator->GetBoundaryEllipsisLength(CutParams.FontSize);
        }
        // After the last fragment
        if (fragment.AppendEllipsis) {
            suffixLengthInPixels += fragment.Calculator->GetBoundaryEllipsisLength(CutParams.FontSize);
        }
        return fragment.Calculator->CalcLengthInRows(
            fragment.TextBeginOfs,
            fragment.TextEndOfs,
            CutParams.FontSize,
            CutParams.PixelsInLine,
            0.0f,
            prefixLengthInPixels,
            suffixLengthInPixels);
    }

    float TSnipLengthCalculator::CalcPixelLengthMulti(const TVector<TTextFragment>& fragments) const {
        float lengthInRows = 0.0f;
        for (size_t i = 0; i < fragments.size(); ++i) {
            const TTextFragment& fragment = fragments[i];
            Y_ASSERT(fragment.Calculator);
            float prefixLengthInPixels = 0.0f;
            float suffixLengthInPixels = 0.0f;
            // Before the first fragment
            if (i == 0 && fragment.PrependEllipsis) {
                prefixLengthInPixels += fragment.Calculator->GetBoundaryEllipsisLength(CutParams.FontSize);
            }
            // Between fragments: space, bold ellipsis, space
            if (i + 1 < fragments.size()) {
                suffixLengthInPixels += fragment.Calculator->GetMiddleEllipsisLength(CutParams.FontSize);
            }
            // After the last fragment
            if (i + 1 == fragments.size() && fragment.AppendEllipsis) {
                suffixLengthInPixels += fragment.Calculator->GetBoundaryEllipsisLength(CutParams.FontSize);
            }
            lengthInRows = fragment.Calculator->CalcLengthInRows(
                fragment.TextBeginOfs,
                fragment.TextEndOfs,
                CutParams.FontSize,
                CutParams.PixelsInLine,
                lengthInRows,
                prefixLengthInPixels,
                suffixLengthInPixels);
        }
        return lengthInRows;
    }

    float TSnipLengthCalculator::CalcSymbolLengthSingle(const TTextFragment& fragment) const {
        size_t length = 0;
        // Before the first fragment
        if (fragment.PrependEllipsis) {
            length += BOUNDARY_ELLIPSIS.length();
        }
        length += fragment.TextEndOfs - fragment.TextBeginOfs;
        // After the last fragment
        if (fragment.AppendEllipsis) {
            length += BOUNDARY_ELLIPSIS.length();
        }
        return static_cast<float>(length);
    }

    float TSnipLengthCalculator::CalcSymbolLengthMulti(const TVector<TTextFragment>& fragments) const {
        size_t length = 0;
        for (size_t i = 0; i < fragments.size(); ++i) {
            const TTextFragment& fragment = fragments[i];
            // Before the first fragment
            if (i == 0 && fragment.PrependEllipsis) {
                length += BOUNDARY_ELLIPSIS.length();
            }
            length += fragment.TextEndOfs - fragment.TextBeginOfs;
            // Between fragments: space, ellipsis, space
            if (i + 1 < fragments.size()) {
                length += MIDDLE_ELLIPSIS.length();
            }
            // After the last fragment
            if (i + 1 == fragments.size() && fragment.AppendEllipsis) {
                length += BOUNDARY_ELLIPSIS.length();
            }
        }
        return static_cast<float>(length);
    }

} // namespace NSnippets
