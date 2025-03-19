#include "list_handler.h"
#include "struct_handler_common.h"

namespace NSnippets {
struct TListItem : public TMarkupSpan {
    ui16 Level;
};

struct TListType : public TMarkupSpan {
    TMarkupSpan HeaderSpan;
    TSpan EffectiveSpan;
    ui16 Level;
    bool Valid;
    TVector<TListItem> Items;
    TListType() : TMarkupSpan(), EffectiveSpan(-1, -1), Level(0), Valid(true), Items() {}
};

const ui16 MIN_LIST_LEVEL = 0;
const ui16 MAX_LIST_LEVEL = 5;

typedef TVector<TListType> TLists;

struct TListArcViewer::TImpl : public TViewerImplBase {
    TLists Lists;

    TImpl() : TViewerImplBase(), Lists() {}
    ~TImpl() override {}

    void OnMarkup(const TArchiveMarkupZones& zones) override {
        BuildLists(zones);
        FillSentOrder();
    }

    void BuildList(const TArchiveMarkupZones& zones, ui16 level) {
        EArchiveZone listZone = Min(static_cast<EArchiveZone>(AZ_LIST0 + level), AZ_LIST5);
        EArchiveZone listItemZone = Min(static_cast<EArchiveZone>(AZ_LIST_ITEM0 + level), AZ_LIST_ITEM5);

        const TVector<TArchiveZoneSpan>& headers = zones.GetZone(AZ_STRICT_HEADER).Spans;
        const TVector<TArchiveZoneSpan>& listSpans = zones.GetZone(listZone).Spans;
        const TVector<TArchiveZoneSpan>& listItemSpans = zones.GetZone(listItemZone).Spans;
        Lists.reserve(listSpans.size());

        size_t curLI = 0;
        for (size_t i = 0; i < listSpans.size(); i++) {
            {
                TListType list;
                list.First = listSpans[i].SentBeg;
                list.Last = listSpans[i].SentEnd;
                list.EffectiveSpan.First = list.First;
                list.EffectiveSpan.Last = list.Last;
                list.Level = level;
                list.HeaderSpan = GetPreviousHeader(list, headers);
                if (list.HeaderSpan.Valid()) {
                    MarkHitsInSpan(list.HeaderSpan, HitSents);
                    list.EffectiveSpan.First = list.HeaderSpan.First;
                }
                Lists.push_back(list);
            }
            TListType& list = Lists.back();

            // some performance insanity
            size_t j = curLI;
            for (; j < listItemSpans.size() && listItemSpans[j].SentEnd <= list.Last; j++) {;}
            list.Items.reserve(j - curLI);

            while (curLI < j) {
                TListItem li;
                li.First = listItemSpans[curLI].SentBeg;
                li.Last = listItemSpans[curLI].SentEnd;
                li.Level = level;

                // check item length
                if (!CheckSpanLen(li)) {
                    list.Valid = false;
                }
                // check list solidness
                if (!list.Items.empty() && list.Items.back().Last != li.First - 1) {
                    list.Valid = false;
                }

                MarkHitsInSpan(li, HitSents);
                list.Items.push_back(li);
                curLI++;
            }
            if (list.Items.empty() || list.First != list.Items.front().First || list.Last != list.Items.back().Last) {
                list.Valid = false;
            }
        }
    }
    void BuildLists(const TArchiveMarkupZones& zones) {
        for (int level = MAX_LIST_LEVEL; level >= MIN_LIST_LEVEL; level--) {
            BuildList(zones, level);
        }
        if (Lists.empty()) {
            return;
        }
        for (size_t i = 0; i < Lists.size() - 1; i++) {
            for(size_t j = i + 1; j < Lists.size(); j++) {
                const TListType& l1 = Lists[i];
                TListType& l2 = Lists[j];
                if (l1.First >= l2.First && l1.Last <= l2.Last) {
                    l2.Valid = false;
                }
            }
        }
    }
    void FillSentOrder() {
        for (size_t i = 0; i < Lists.size(); i++) {
            AddHeaderSpan(Lists[i].HeaderSpan, HitOrderGen);
            for (size_t j = 0; j < Lists[i].Items.size(); j++) {
                const TListItem& item = Lists[i].Items[j];
                if (CheckSpanLen(item)) {
                    HitOrderGen.PushBack(item.First, item.Last);
                }
            }
        }
    }

    std::pair<NStruct::TLICoord, NStruct::TLICoord> CheckList(const TSpan sentSpan) const {
        int listID = -1;
        for (int i = 0; i < Lists.ysize(); i++) {
            if (Lists[i].Valid && Lists[i].EffectiveSpan.Contains(sentSpan)) {
                listID = i;
                int firstItem = -1;
                int lastItem = -1;
                for (int j = 0; j < Lists[i].Items.ysize(); j++) {
                    const TListItem& li = Lists[i].Items[j];
                    if (li.Contains(sentSpan.First)) {
                        firstItem = j;
                    }
                    if (li.Contains(sentSpan.Last)) {
                        lastItem = j;
                        break;
                    }
                }
                if (firstItem == -1) {
                    firstItem = 0;
                }
                if (firstItem <= lastItem) {
                    return {{listID, firstItem}, {listID, lastItem}};
                }
            }
        }
        return {NStruct::BAD_PAIR, NStruct::BAD_PAIR};
    }
    bool HasListHeader(ui16 list) const {
        if (list >= Lists.size()) {
            return false;
        }
        return Lists[list].HeaderSpan.Valid();
    }
    void FillListInfo(ui16 list, THolder<NStruct::TListInfo>& info) const {
        info.Reset(new NStruct::TListInfo());
        if (list < Lists.size()) {
            const TListType& l = Lists[list];
            info->ItemCount = l.Items.size();
            if (l.HeaderSpan.Valid() && CheckSpanLen(l.HeaderSpan)) {
                info->HasHeader = true;
                info->HeaderSpan.First = l.HeaderSpan.First;
                info->HeaderSpan.Last = l.HeaderSpan.Last;
                info->HasHeaderHits = l.HeaderSpan.ContainHits;
            }
            info->Span.First = l.First;
            info->Span.Last = l.Last;
            info->Valid = l.Valid;
            info->ItemSpans.reserve(l.Items.size());
            for (size_t i = 0; i < l.Items.size(); i++) {
                info->ItemSpans.push_back(TSpan(l.Items[i].First, l.Items[i].Last));
                if (l.Items[i].ContainHits) {
                    info->HitItems.push_back(i);
                }
            }
        }
    }
};

TListArcViewer::TListArcViewer() : Impl(new TListArcViewer::TImpl()) {}
TListArcViewer::~TListArcViewer() {}
const TSentsOrder& TListArcViewer::GetSentsOrder() const {
    return Impl->GetSentsOrder();
}
void TListArcViewer::OnHitsAndSegments(const TVector<ui16>& hitSents, const NSegments::TSegmentsInfo*) {
    Impl->OnHits(hitSents);
}
void TListArcViewer::OnUnpacker(TUnpacker* unpacker) {
    Impl->OnUnpacker(unpacker);
}
void TListArcViewer::OnBeforeSents() {
    Impl->OnBeforeSents();
}
void TListArcViewer::OnMarkup(const TArchiveMarkupZones& zones) {
    Impl->OnMarkup(zones);
}
void TListArcViewer::OnEnd() {}

bool TListArcViewer::HasLists() const {
    return !Impl->Lists.empty();
}
bool TListArcViewer::HasListHeader(ui16 list) const {
    return Impl->HasListHeader(list);
}
std::pair<NStruct::TLICoord, NStruct::TLICoord> TListArcViewer::CheckList(const TSpan sentSpan) const {
    return Impl->CheckList(sentSpan);
}
void TListArcViewer::FillListInfo(ui16 list, THolder<NStruct::TListInfo>& info) const {
    Impl->FillListInfo(list, info);
}
}
