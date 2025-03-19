#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

    class TTranslatedDocReplacer : public IReplacer {
    public:
        TTranslatedDocReplacer();
        void DoWork(TReplaceManager* manager) override;

    private:
        void AddTranslatedDocSnippet(TReplaceManager* manager) const;
        void AddTranslatedOriginalDocSnippet(TReplaceManager* manager) const;
    };

} // namespace NSnippets
