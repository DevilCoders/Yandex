#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

    class TISnipReplacer : public IReplacer {
        public:
            TISnipReplacer();
            void DoWork(TReplaceManager* manager) override;
    };

} // namespace NSnippets
