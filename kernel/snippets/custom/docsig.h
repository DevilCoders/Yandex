#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TDocSigDataReplacer : public IReplacer {
public:
    TDocSigDataReplacer()
        : IReplacer("docsigdata")
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}

