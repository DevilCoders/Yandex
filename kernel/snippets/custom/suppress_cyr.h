#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TCyrillicToEmptyReplacer : public IReplacer {
public:
    TCyrillicToEmptyReplacer()
        : IReplacer("Cyrillic_to_empty")
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
