#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TTableArcViewer;

class TTableSnipReplacer : public IReplacer {
private:
    const TTableArcViewer& ArcViewer;
public:
    TTableSnipReplacer(const TTableArcViewer& arcViewer)
        : IReplacer("table_snip")
        , ArcViewer(arcViewer)
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
