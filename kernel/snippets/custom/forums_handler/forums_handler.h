#pragma once

#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/chooser/chooser.h>
#include <kernel/snippets/iface/archive/viewer.h>

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcface.h>

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/string.h>
#include <util/generic/hash.h>

namespace NSnippets
{

class TUnpacker;

struct TForumMessageZone
{
    TArchiveZoneSpan Span;
    TUtf16String Anchor;
    TUtf16String Date;
    TUtf16String Content;
    TUtf16String Author;
    TUtf16String LongContent;
    size_t Position = 0;
    size_t Popularity = 0;
    bool HasPopularity = false;

    TArchiveView Sents;
    size_t Len = 0;

    TForumMessageZone(const TArchiveZoneSpan& span)
        : Span(span)
    {
    }

    bool HasSent(int sent) {
        return Span.SentBeg <= sent && sent <= Span.SentEnd;
    }
};


TUtf16String ExtractForumAttr(const THashMap<TString, TUtf16String>* attributes, const char* attrName);
int GuessLastSent(const TArchiveMarkupZones& zones);

class TForumMarkupViewer : public IArchiveViewer {
public:
    typedef TVector<TForumMessageZone> TZones;
    enum EForumPageKind
    {
        NONE,
        MESSAGES,
        THREADS,
        FORUMS
    };

    TSentsOrder All;
    EForumPageKind PageKind;
    TZones ForumZones;
    TUtf16String ForumTitle;
    bool HasValidMarkup;
    bool HasValidAttrs;
    bool MixedPageKind;
    int PageNum;
    int TotalNumMessages;
    int NumItems;
    bool NumItemsPrecise;
    int NumPages;
    int NumItemsOnPage;
    bool FilterByPosition;
    TUnpacker* Unpacker;
    TVector<TArchiveZoneSpan> MessageSpans;
    TVector<TArchiveZoneSpan> QuoteSpans;
    TVector<bool> ForumMessageSents;

public:
    TForumMarkupViewer(bool enable, const TDocInfos& infos, bool filterByFirstPosition = true);

    void OnEnd() override {
        TArchiveView all;
        DumpResult(All, all);
        size_t i = 0;
        TZones::iterator j = ForumZones.begin();
        while (i != all.Size() && j != ForumZones.end()) {
            while (i != all.Size() && (*j).HasSent(all.Get(i)->SentId)) {
                (*j).Sents.PushBack(all.Get(i));
                (*j).Len += all.Get(i)->Sent.size();
                ++i;
            }
            while (i != all.Size() && all.Get(i)->SentId < (*j).Span.SentBeg) {
                ++i;
            }
            if (i == all.Size()) {
                break;
            }
            while (j != ForumZones.end() && (*j).Span.SentEnd < all.Get(i)->SentId) {
                ++j;
            }
        }
    }

    void OnUnpacker(TUnpacker* unpacker) override {
        Unpacker = unpacker;
    }

    bool ZoneIn(const TArchiveZoneSpan& outer, const TArchiveZoneSpan& inner);
    void AddZoneSpans(const TArchiveZone& zone);
    void ZonesToForumItems(const TArchiveMarkupZones& zones);

    void OnMarkup(const TArchiveMarkupZones& zones) override;

    const TVector<bool>& GetForumMessageSents() const {
        return ForumMessageSents;
    }

    bool IsValid() const {
        return HasValidMarkup && HasValidAttrs;
    }

    bool FilterByMessages() const {
        return IsValid() && PageKind == MESSAGES;
    }

    void FillSentExcludeFilter(TSentFilter& filter, const TArchiveView& sents) const;

};

}
