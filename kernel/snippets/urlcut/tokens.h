#pragma once

#include "matcher.h"

#include <util/generic/hash_set.h>
#include <util/generic/list.h>
#include <util/generic/string.h>

namespace NUrlCutter {
    class THilitedString;

    enum ETokenType {
        TT_NONE,
        TT_DATE,
        TT_WORD
    };

    enum EPathType {
        PT_DOMAIN,
        PT_PATH_SP,
        PT_PATH,
        PT_CGI_SP,
        PT_CGI
    };

    enum ETokenStatus {
        TS_NONE,
        TS_USED,
        TS_REMOVED
    };

    typedef TDeque<TQueryWord> TQWList;

    struct TSpan {
        ui32 Begin;
        ui32 End;
        bool IsStop;

        TSpan(ui32 pos, size_t len, bool isStop);
        bool TryMerge(const TSpan& next);
    };

    typedef TVector<TSpan> TSpanVector;

    class TTokenInfo {
    public:
        TUtf16String Token;
        ETokenType TType;
        EPathType PType;
        TQWList Matches;
        size_t Len;
        ETokenStatus TStatus;
        i32 AddRevFreq;
        i32 Gray;
        TSpanVector HiliteSpans;

    public:
        TTokenInfo(const TUtf16String& token, ETokenType tt, EPathType pt, const TMatcherSearcher::TSearchResult* sr = nullptr);
        i64 GetFreq(THashSet<TUtf16String>& used, bool wGray = true) const;
        void FillUsed(THashSet<TUtf16String>& used) const;
        void SetStatus(ETokenStatus tstatus, THashSet<TUtf16String>& used);
        bool Update(TTokenInfo& prev, bool wasSp, const THashSet<TUtf16String>& DynamicStopWords);
        THilitedString GetHilitedToken() const;
        void Relax();
        bool IsTotalHilite() const;
    };

    typedef TList<TTokenInfo> TTokenList;
}
