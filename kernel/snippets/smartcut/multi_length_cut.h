#pragma once
#include <util/generic/string.h>

namespace NSnippets
{
    struct TMultiCutResult
    {
        TUtf16String Short;
        TUtf16String Long;
        int CommonPrefixLen = 0;
        int CharCountDifference = 0; // this ignores ellipsis, so can be different from Long.size() - Short.size()
        int WordCountDifference = 0;

        explicit TMultiCutResult(const TUtf16String& shortOnly)
            : Short(shortOnly)
        {
        }

        TMultiCutResult() = default;
        TMultiCutResult(const TMultiCutResult& other) = default;
        TMultiCutResult(const TUtf16String& shortStr, const TUtf16String& longStr);

        explicit operator bool() const;
    };
}
