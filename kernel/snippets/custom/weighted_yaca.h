#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/list.h>

namespace NSnippets
{
    class TReplaceContext;
    struct TSpecSnippetCandidate;

    class TWeightedYacaCandidateProducer
    {
    private:
        const TReplaceContext& Context;

    public:
        TWeightedYacaCandidateProducer(const TReplaceContext& context)
            : Context(context)
        {
        }

        void Produce(ELanguage lang, TList<TSpecSnippetCandidate>& candidates);
    };
}
