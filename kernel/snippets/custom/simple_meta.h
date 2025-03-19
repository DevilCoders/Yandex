#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {
    class TMetaDescrReplacer: public IReplacer {
    public:
        TMetaDescrReplacer()
          : IReplacer("simple_meta_descr")
        {
        }
        void DoWork(TReplaceManager* manager) override;
    };
}

