#pragma once

#include <kernel/snippets/replace/replace.h>
#include <library/cpp/json/json_value.h>

namespace NSnippets {

class TSchemaOrgArchiveViewer;

class TQuestionReplacer : public IReplacer {
private:
    const TSchemaOrgArchiveViewer& ArcViewer;

public:
    TQuestionReplacer(const TSchemaOrgArchiveViewer& arcViewer);
    void DoWork(TReplaceManager* manager) override;
};

} // namespace NSnippets
