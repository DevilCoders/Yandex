#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/list.h>
#include <util/generic/ptr.h>

namespace NSnippets
{
    class TReplaceContext;
    struct TSpecSnippetCandidate;
    class TDmozData;

    class TWeightedDmozCandidateProducer
    {
    private:
        THolder<TDmozData> DmozData;
        const TReplaceContext& Context;

    public:
        TWeightedDmozCandidateProducer(const TReplaceContext& context);
        ~TWeightedDmozCandidateProducer();

        void Produce(ELanguage lang, TList<TSpecSnippetCandidate>& candidates);
    };
}
