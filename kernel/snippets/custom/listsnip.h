#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TListArcViewer;

class TListSnipData {
public:
    int ListHeader;
    int ItemCount;
    int First;
    int Last;
public:
    TListSnipData();
    TString Dump(bool dropStat) const;
};

class TSnipListReplacer : public IReplacer {

private:
    const TListArcViewer& ArcViewer;

public:
    TSnipListReplacer(const TListArcViewer& arcViewer)
        : IReplacer("list_snip")
        , ArcViewer(arcViewer)
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
