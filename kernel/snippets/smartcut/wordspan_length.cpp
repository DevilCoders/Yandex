#include "wordspan_length.h"
#include "snip_length.h"

namespace NSnippets
{
    TWordSpanLengthCalculator::TWordSpanLengthCalculator(
            TMakeFragment makeFragment, const TSnipLengthCalculator& calculator)
        : MakeFragment(makeFragment)
        , Calculator(calculator)
    {
    }

    int TWordSpanLengthCalculator::FindFirstWordLonger(
            int firstWord, int lastWord, float maxLength) const {
        int left = firstWord;
        int right = lastWord + 1;
        while (left < right) {
            int center = (left + right) / 2;
            if (Calculator.CalcLengthSingle(MakeFragment(firstWord, center)) <= maxLength) {
                left = center + 1;
            } else {
                right = center;
            }
        }
        return left;
    }

} // namespace NSnippets
