#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TSocnetDataReplacer : public IReplacer {
public:
    TSocnetDataReplacer()
        : IReplacer("socnetsnipdata")
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
