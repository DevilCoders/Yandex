#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TWeightedIdealCatalogReplacer : public IReplacer {
public:
    TWeightedIdealCatalogReplacer()
        : IReplacer("weighted_icatalog")
    {
    }
    void DoWork(TReplaceManager* manager) override;
};


class TWeightedYaCatalogReplacer : public IReplacer {
public:
    TWeightedYaCatalogReplacer()
        : IReplacer("weighted_yaca")
    {
    }
    void DoWork(TReplaceManager* manager) override;
};

class TWeightedMetaDescrReplacer : public IReplacer {
public:
    TWeightedMetaDescrReplacer()
        : IReplacer("weighted_metadescr")
    {
    }
    void DoWork(TReplaceManager* manager) override;
};

class TWeightedVideoDescrReplacer : public IReplacer {
public:
    TWeightedVideoDescrReplacer()
        : IReplacer("weighted_videodescr")
    {
    }
    void DoWork(TReplaceManager* manager) override;
};

}
