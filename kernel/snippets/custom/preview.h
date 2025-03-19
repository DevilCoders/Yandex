#pragma once

#include <kernel/snippets/replace/replace.h>

#include <util/generic/ptr.h>

namespace NSnippets {
    //forward declarations
    class TForumMarkupViewer;
    class TContentPreviewViewer;

    class TPreviewReplacer : public IReplacer {
    private:
        class TImpl;

        THolder<TImpl> Impl;
    public:
        TPreviewReplacer(const TContentPreviewViewer& contentPreviewViewer, const TForumMarkupViewer& forums);
        ~TPreviewReplacer() override;

        void DoWork(TReplaceManager* manager) override;
    };
}

