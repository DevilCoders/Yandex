#pragma once

#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/weight/weighter_decl.h>

namespace NSnippets
{
    class TConfig;
    class TSnip;
    class TUniSpanIter;
    class TSentsMatchInfo;
    struct ISnippetCandidateDebugHandler;
    namespace NSnipVideoWordSpans
    {
        TSnip GetVideoSnippet(const TConfig& cfg, TMxNetWeighter& w, TUniSpanIter& sF, const TSentsMatchInfo& smInfo, float maxSize, ECandidateSource source, ISnippetCandidateDebugHandler* cb, const char* aname);
    }

}
