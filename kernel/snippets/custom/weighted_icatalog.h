#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/list.h>

namespace NSnippets
{
    class TReplaceContext;
    struct TSpecSnippetCandidate;

    class TWeightedIdealCatalogCandidateProducer
    {
    private:
        const TReplaceContext& Context;

    public:
        TWeightedIdealCatalogCandidateProducer(const TReplaceContext& context)
            : Context(context)
        {
        }

        void Produce(ELanguage lang, TList<TSpecSnippetCandidate>& candidates);
    };
}
