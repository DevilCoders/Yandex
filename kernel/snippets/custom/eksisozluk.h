#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TSchemaOrgArchiveViewer;

class TEksisozlukReplacer : public IReplacer {
private:
    const TSchemaOrgArchiveViewer& ArcViewer;

public:
    TEksisozlukReplacer(const TSchemaOrgArchiveViewer& arcViewer);

    void DoWork(TReplaceManager* manager) override;
};

} // namespace NSnippets
