#pragma once

#include <util/generic/string.h>
#include <util/string/vector.h>
#include <util/charset/wide.h>

#include <algorithm>

namespace NEditDistance {
    using VectorParWtrok = TVector<std::pair<TUtf16String, TUtf16String>>;
    const int MAX_DIFF = 1e6;

    TVector<size_t> GetWordLengths(TUtf16String& wtroka);
    TVector<TUtf16String> RecoverStringsByLengths(const TUtf16String& wtroka, const TVector<size_t> lengths);
    TUtf16String ChangeSymbol(const TUtf16String& wtroka, size_t index, wchar16 symbol);
    TUtf16String RemoveSequentialSeparators(const TUtf16String& wtroka, const wchar16* separator);
    size_t CalculateDifference(const VectorParWtrok& wtroki);
    VectorParWtrok AlignWtrokasOfDifferentLength(const TUtf16String& lhs, const TUtf16String& rhs);
    VectorParWtrok AlignWtrokas(const TUtf16String& lhs, const TUtf16String& rhs);
}
