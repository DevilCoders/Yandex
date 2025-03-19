#pragma once

#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/weight/weighter_decl.h>

namespace NSnippets
{
    class TSnip;
    class TUniSpanIter;
    class TSentsMatchInfo;
    struct ISnippetCandidateDebugHandler;
    namespace NSnipWordSpans
    {
        //- 1 snippet
        //- not necessarily whole sentences, but tends to use whole ones
        TSnip GetTSnippet(TMxNetWeighter& w, TUniSpanIter& sF, const TSentsMatchInfo& smInfo, float maxSize, ECandidateSource source, ISnippetCandidateDebugHandler* cb,
                          ISnippetCandidateDebugHandler* fsccb, const char* aname, bool needCheckTitle = false, float repeatedTitlePessimizeFactor = 0.0);
    }

}
