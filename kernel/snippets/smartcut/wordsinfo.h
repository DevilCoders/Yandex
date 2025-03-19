#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NSnippets
{
    class TTextFragment;

    class TWordsInfo {
    public:
        struct TWordNode {
            size_t WordBegin = 0;
            size_t WordEnd = 0;
            size_t TextBufBegin = 0;
            size_t TextBufEnd = 0;
            bool FirstInSent = false;
            bool LastInSent = false;
        };
        TVector<TWordNode> Words;
        TUtf16String Text;

    public:
        TWordsInfo(const TUtf16String& source, const wchar16* const hilightMark);
        int WordCount() const;
        TWtringBuf GetWordBuf(int wordId) const;
        TWtringBuf GetPunctAfter(int wordId) const;
        TWtringBuf GetWordWithPunctAfter(int wordId) const;
        TWtringBuf GetTextBuf(int firstWordId, int lastWordId) const;
        TTextFragment GetTextFragment(int firstWordId, int lastWordId) const;
    };
}
