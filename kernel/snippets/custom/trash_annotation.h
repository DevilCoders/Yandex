#pragma once

#include <kernel/snippets/replace/replace.h>

#include <util/generic/string.h>

namespace NSnippets {
    class TConfig;
    class TTrashViewer;
    class TSnipTitle;
    class TSnip;
    class TArchiveMarkup;

    class TTrashAnnotationReplacer : public IReplacer {
    private:
        static const TString TEXT_SRC;

        const TTrashViewer& TrashViewer;

    public:
        TTrashAnnotationReplacer(const TTrashViewer& trashViewer)
            : IReplacer("trash_annotation")
            , TrashViewer(trashViewer)
        {
        }

        void DoWork(TReplaceManager* manager) override;
    };
}

