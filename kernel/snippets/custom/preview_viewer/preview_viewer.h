#pragma once

#include <kernel/tarc/docdescr/docdescr.h>

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>

namespace NSchemaOrg {
    class TTreeNode;
}

namespace NSnippets {
    //forward declarations
    class TConfig;
    class IArchiveViewer;
    class TArchiveView;

    class TContentPreviewViewer {
    public:
        TContentPreviewViewer(const TConfig& cfg, const TDocInfos& docInfos);
        ~TContentPreviewViewer();

    public:
        IArchiveViewer& GetViewer();
        const TArchiveView& GetResult() const;
        const TVector<TArchiveView>& GetAdditionalResults() const;
        const NSchemaOrg::TTreeNode* GetSchema() const;
        const THashSet<int>& GetMainContent() const;
        const THashSet<int>& GetGoodContent() const;
        int GuessSentCount() const;
        int GetContentSentCount() const;
        bool IsForumDoc() const;
        const TVector<std::pair<ui16, ui16>>& GetForumQuoteSpans() const;

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };
}

