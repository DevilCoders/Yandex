#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets
{
    class TStatAnnotViewer;

    class TStaticExtendedReplacer : public IReplacer {
    private:
        const TStatAnnotViewer& StatAnnotViewer;

    public:
        TStaticExtendedReplacer(const TStatAnnotViewer& statAnnotViewer)
            : IReplacer("static_extended")
            , StatAnnotViewer(statAnnotViewer)
        {
        }

        void DoWork(TReplaceManager* manager) override;
    };
}
