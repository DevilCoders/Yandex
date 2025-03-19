#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TSnip;

class TRobotsTxtStubReplacer : public IReplacer {
private:
    bool IsFakeForBan;
public:
    TRobotsTxtStubReplacer(bool isFakeForBan);
    void DoWork(TReplaceManager* manager) override;
};

}
