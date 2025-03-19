#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TSchemaOrgArchiveViewer;

class TSoftwareApplicationReplacer : public IReplacer {
private:
    const TSchemaOrgArchiveViewer& ArcViewer;
public:
    TSoftwareApplicationReplacer(const TSchemaOrgArchiveViewer& arcViewer);
    void DoWork(TReplaceManager* manager) override;
};

} // namespace NSnippets
