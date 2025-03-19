#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

TSnip GetExtendedIfLongEnough(const TReplaceContext& repCtx, float maxLen, const TSnip& source, const TSnip& natural, bool experimentalMeasure);

class TExtendedSnippetDataReplacer : public IReplacer {
    const TArchiveView& ExtTextView;

public:
    TExtendedSnippetDataReplacer(const TArchiveView& extTextView)
        : IReplacer("extsnipdata")
        , ExtTextView(extTextView)
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
