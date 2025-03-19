#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TSchemaOrgArchiveViewer;

class TYoutubeChannelReplacer: public IReplacer {
private:
    const TSchemaOrgArchiveViewer& ArcViewer;
public:
    TYoutubeChannelReplacer(const TSchemaOrgArchiveViewer& arcViewer);
    void DoWork(TReplaceManager* manager) override;
};

};
