#pragma once

#include <kernel/snippets/replace/replace.h>

#include <util/generic/string.h>

namespace NSnippets {
    //forward declarations
    class TStatAnnotViewer;

    class TStaticAnnotationReplacer : public IReplacer {
    private:
        static const TString TEXT_SRC;

        const TStatAnnotViewer& StatAnnotViewer;

    public:
        TStaticAnnotationReplacer(const TStatAnnotViewer& statAnnotViewer)
            : IReplacer("static_annotation")
            , StatAnnotViewer(statAnnotViewer)
        {
        }

        void DoWork(TReplaceManager* manager) override;
    };
}

