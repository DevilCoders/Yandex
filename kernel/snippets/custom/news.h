#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {
    class TNewsReplacer : public IReplacer
    {
    public:
        void DoWork(TReplaceManager* manager) override;
        TNewsReplacer()
            : IReplacer("news")
        {
        }
    };
}
