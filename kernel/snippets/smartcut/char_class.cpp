#include "char_class.h"

#include <util/charset/unidata.h>

#define SHIFT(i) (ULL(1)<<(i))

namespace NSnippets
{
    bool IsLeftQuoteOrBracket(wchar16 c)
    {
        return NUnicode::CharHasType(c,
            SHIFT(Ps_START)
            | SHIFT(Ps_QUOTE)
            | SHIFT(Po_QUOTE)
            | SHIFT(Pi_QUOTE)
            | SHIFT(Ps_SINGLE_QUOTE)
            | SHIFT(Po_SINGLE_QUOTE)
            | SHIFT(Pi_SINGLE_QUOTE)
            );
    }

    bool IsRightQuoteOrBracket(wchar16 c)
    {
        const wchar16 LEFT_DOUBLE_QUOTATION_MARK = 0x201C; // Often used as a right quote in Russian
        return NUnicode::CharHasType(c,
            SHIFT(Pe_END)
            | SHIFT(Pe_QUOTE)
            | SHIFT(Po_QUOTE)
            | SHIFT(Pe_SINGLE_QUOTE)
            | SHIFT(Po_SINGLE_QUOTE)
            | SHIFT(Pf_QUOTE)
            | SHIFT(Pf_SINGLE_QUOTE)
            ) || c == LEFT_DOUBLE_QUOTATION_MARK;
    }

} // namespace NSnippets

#undef SHIFT
