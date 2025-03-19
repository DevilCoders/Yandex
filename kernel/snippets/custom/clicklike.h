#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TClickLikeSnipDataReplacer : public IReplacer {
public:
    TClickLikeSnipDataReplacer()
        : IReplacer("clicklikesnipdata")
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
