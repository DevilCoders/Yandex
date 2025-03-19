#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TFakeRedirectReplacer : public IReplacer {
private:
    const bool IsFakeForRedirect;
public:
    TFakeRedirectReplacer(bool isFakeForRedirect);
    void DoWork(TReplaceManager* manager) override;
};

}
