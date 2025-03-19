#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TArchiveView;

class TEntityDataReplacer : public IReplacer {
private:
    const TArchiveView& EntityView;
public:
    explicit TEntityDataReplacer(const TArchiveView& entityView)
      : IReplacer("entitydata")
      , EntityView(entityView)
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
