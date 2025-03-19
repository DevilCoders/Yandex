#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TRobotDateReplacer : public IReplacer {
public:
    TRobotDateReplacer();
    void DoWork(TReplaceManager* manager) override;
};

} // namespace NSnippets
