#pragma once

#include "snip_length.h"
#include <util/generic/ptr.h>
#include <functional>

namespace NSnippets
{
    class TSnipLengthCalculator;

    using TMakeFragment = std::function<TTextFragment(int, int)>;

    class TWordSpanLengthCalculator
    {
    private:
        TMakeFragment MakeFragment;
        const TSnipLengthCalculator& Calculator;

    public:
        TWordSpanLengthCalculator(TMakeFragment makeFragment, const TSnipLengthCalculator& calculator);

        // Returns:
        // - the index of the first word in the range from firstWord to lastWord
        //   that doesn't fit in the given length
        // - or lastWord + 1 if the whole text fits in the given length
        int FindFirstWordLonger(int firstWord, int lastWord, float maxLength) const;
    };
}
