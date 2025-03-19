#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TNeedTranslateReplacer : public IReplacer {
public:
    TNeedTranslateReplacer();
    void DoWork(TReplaceManager* manager) override;
};

} // namespace NSnippets
