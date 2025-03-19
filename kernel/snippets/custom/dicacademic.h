#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {
    class TDicAcademicReplacer : public IReplacer {
    public:
        TDicAcademicReplacer()
            : IReplacer("dicacademic")
        {
        }
        void DoWork(TReplaceManager* manager) override;
    };
}
