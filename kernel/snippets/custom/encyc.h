#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets
{
    class TEncycReplacer : public IReplacer {
    public:
        void DoWork(TReplaceManager* manager) override;
        TEncycReplacer()
            : IReplacer("encyc")
        {
        }
    };
}
