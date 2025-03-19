#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {
    class TKinoPoiskReplacer : public IReplacer
    {
    public:
        void DoWork(TReplaceManager* manager) override;
        TKinoPoiskReplacer()
            : IReplacer("kinopoisk")
        {
        }
    };
}
