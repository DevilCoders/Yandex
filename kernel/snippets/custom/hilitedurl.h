#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class THilitedUrlDataReplacer : public IReplacer {
public:
    THilitedUrlDataReplacer()
      : IReplacer("hilitedurldata")
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
