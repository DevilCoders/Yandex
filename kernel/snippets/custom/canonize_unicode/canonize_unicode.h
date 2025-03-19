#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TCanonizeUnicodeReplacer : public IReplacer {
public:
    TCanonizeUnicodeReplacer()
        : IReplacer("canonize_unicode")
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

bool CanonizeUnicode(TUtf16String& str);
}
