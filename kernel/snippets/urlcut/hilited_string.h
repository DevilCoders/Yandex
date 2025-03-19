#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NUrlCutter {
    class THilitedString {
    public:
        TUtf16String String;
        TVector<std::pair<size_t, size_t>> SortedHilitedSpans;

    public:
        THilitedString();
        explicit THilitedString(const TUtf16String& str);

        THilitedString& Append(const THilitedString& hs);
        THilitedString& Append(const TUtf16String& str);
        TUtf16String Merge(const TUtf16String& openMark, const TUtf16String& closeMark) const;
        void FillHilitedWords(TVector<TUtf16String>& hlWords) const;
        TVector<TUtf16String> GetHilitedWords() const;
    };
}
