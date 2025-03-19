#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {
    class TMarketReplacer : public IReplacer {
    public:
        void DoWork(TReplaceManager* manager) override;
        TMarketReplacer()
            : IReplacer("market")
        {
        }
    };
}
