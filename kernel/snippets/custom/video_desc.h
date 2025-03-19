#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {
    class TVideoReplacer : public IReplacer {
    public:
        TVideoReplacer()
            : IReplacer("video")
        {
        }
        void DoWork(TReplaceManager* manager) override;
    };
}
