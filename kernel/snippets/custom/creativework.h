#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {
    class TSchemaOrgArchiveViewer;
    class TCreativeWorkReplacer : public IReplacer {
    private:
        const TSchemaOrgArchiveViewer& ArcViewer;
    public:
        void DoWork(TReplaceManager* manager) override;
        TCreativeWorkReplacer(const TSchemaOrgArchiveViewer& arcViewer);
    };
}
