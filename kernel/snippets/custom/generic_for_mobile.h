#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TGenericForMobileDataReplacer : public IReplacer {
public:
    TGenericForMobileDataReplacer()
        : IReplacer("genericmobiledata")
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
