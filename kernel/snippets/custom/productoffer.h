#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {
    class TSchemaOrgArchiveViewer;

    class TProductOfferReplacer : public IReplacer {
        const TSchemaOrgArchiveViewer& ArcViewer;
    public:
        TProductOfferReplacer(const TSchemaOrgArchiveViewer& arcViewer);
        void DoWork(TReplaceManager* manager) override;
    };
}
