#include "retained_info.h"
#include "tsnip.h"

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/iface/archive/enums.h>

namespace NSnippets {
    TRetainedSentsMatchInfo::TRetainedSentsMatchInfo() {
    }

    TRetainedSentsMatchInfo::~TRetainedSentsMatchInfo() {
    }

    void TRetainedSentsMatchInfo::CreateSentsMatchInfo(const TArchiveMarkup* markup, const TArchiveView& view, const TParams& params) {
        THolder<TSentsInfo> si(new TSentsInfo(markup, view, params.MetaDescrAdd, params.PutDot, params.ParaTables));
        SentsInfo.Reset(si.Release());
        SentsMatchInfo.Reset(new TSentsMatchInfo(*SentsInfo, params.Query, params.Cfg, params.DocLangId));
    }

    void TRetainedSentsMatchInfo::SetView(const TUtf16String& source, const TParams& params) {
        Storage.Reset(new TArchiveStorage());
        TArchiveView view;
        view.PushBack(&*Storage->Add(ARC_MISC, 0, source, 0));
        CreateSentsMatchInfo(nullptr, view, params);
    }

    void TRetainedSentsMatchInfo::SetView(const TVector<TUtf16String>& source, const TParams& params) {
        Storage.Reset(new TArchiveStorage());
        TArchiveView view;
        for (int i = 0; i < source.ysize(); ++i) {
            view.PushBack(&*Storage->Add(ARC_MISC, i, source[i], 0));
        }
        CreateSentsMatchInfo(nullptr, view, params);
    }

    void TRetainedSentsMatchInfo::SetView(const TArchiveMarkup* markup, const TArchiveView& view, const TParams& params) {
        CreateSentsMatchInfo(markup, view, params);
    }

    TSnip TRetainedSentsMatchInfo::AllAsSnip() const {
        TSnip snip;
        for (int i = 0; i < SentsInfo->SentencesCount(); ++i) {
            if (SentsInfo->IsSentIdFirstInArchiveSent(i)) {
                int wordBeg = SentsInfo->FirstWordIdInSent(i);
                int wordEnd = SentsInfo->LastWordIdInSent(i);
                snip.Snips.push_back(TSingleSnip(wordBeg, wordEnd, *SentsMatchInfo));
            } else {
                int wordEnd = SentsInfo->LastWordIdInSent(i);
                snip.Snips.back().SetWordRange(snip.Snips.back().GetFirstWord(), wordEnd);
            }
        }
        return snip;
    }

    TCustomSnippetsStorage::TCustomSnippetsStorage() {
    }

    TCustomSnippetsStorage::~TCustomSnippetsStorage() {
    }

    TRetainedSentsMatchInfo& TCustomSnippetsStorage::CreateRetainedInfo() {
        RetainedInfos.push_back(TRetainedSentsMatchInfo());
        return RetainedInfos.back();
    }

    void TCustomSnippetsStorage::RetainCopy(const TRetainedSentsMatchInfo& retainedInfo) {
        RetainedInfos.push_back(retainedInfo);
    }
}
