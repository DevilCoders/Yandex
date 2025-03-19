#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>

struct TZonedString;

namespace NSnippets {
    class TZonedStringTransformer {
        TZonedString& Z;
        TVector<ssize_t> NewBeg;
        TVector<ssize_t> NewEnd;
        int CurIdx = 0;
        TUtf16String S;
        bool SkipZeroLenSpans = true;
    public:
        TZonedStringTransformer(TZonedString& z, bool skipZeroLenSpans = true);
        void Step(int cnt = 1);
        void Delete(int cnt);
        void Replace(const TUtf16String& x);
        void InsertBeforeNext(const TUtf16String& x);
        void Complete();
    };
}

