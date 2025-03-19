#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TConfig;
class TSchemaOrgArchiveViewer;

class TSchemaOrgRatingReplacer : public IReplacer {
private:
    const TSchemaOrgArchiveViewer& ArcViewer;
public:
    explicit TSchemaOrgRatingReplacer(const TSchemaOrgArchiveViewer& viewer)
        : IReplacer("schemaorg_rating")
        , ArcViewer(viewer)
    {
    }

    static bool HasRating(const TConfig& cfg, const TSchemaOrgArchiveViewer& viewer);

    void DoWork(TReplaceManager* manager) override;
};

}

