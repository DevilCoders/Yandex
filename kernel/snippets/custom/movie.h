#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

    class TSchemaOrgArchiveViewer;

    class TMovieReplacer : public IReplacer {
    private:
        const TSchemaOrgArchiveViewer& ArcViewer;

    public:
        TMovieReplacer(const TSchemaOrgArchiveViewer& arcViewer);

        void DoWork(TReplaceManager* manager) override;
    };
}
