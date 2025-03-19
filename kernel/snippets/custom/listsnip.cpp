#include "listsnip.h"
#include "list_handler.h"
#include "struct_common.h"

#include <kernel/snippets/algo/one_span.h>
#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/zone_checker/zone_checker.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/wordstat/wordstat.h>
#include <kernel/snippets/wordstat/wordstat_data.h>

#include <kernel/tarc/iface/tarcface.h>

#include <kernel/segmentator/structs/segment_span.h>

namespace NSnippets {

typedef TVector<NStruct::TLICoord> TItems;

static const char* const LIST_SNIPPETS = "list_snip";
static const char* const LIST_DATA = "listData";
static const int MIN_ITEM_CNT = 2;
static const int MAX_ITEM_CNT = 3;

TListSnipData::TListSnipData()
    : ListHeader(-1)
    , ItemCount(0)
    , First(-1)
    , Last(-1)
{
}

TString TListSnipData::Dump(bool dropStat) const {
    TStringStream stream;
    stream << "[{";
    stream << "\"if\":" << First << ",\"il\":" << Last;
    if (ListHeader > -1) {
        stream << ",\"lh\":" << ListHeader;
    }
    if (!dropStat) {
        stream << ",\"lic\":" << ItemCount;
    }
    stream << "}]";
    return stream.Str();
}

struct TListSnipCtx {
    int ListID = -1;
    int TextPartCnt = 0;
    int TextPartStart = -1;
    std::pair<int, int> TextWordRange = {-1, 1};
    float TextPartLen = 0.0f;
    TItems Items;
    THolder<NStruct::TListInfo> ListInfo;
    TItems BestHit;
    const TReplaceContext& RepCtx;
public:
    explicit TListSnipCtx(const TReplaceContext& repCtx)
        : RepCtx(repCtx)
    {
    }
    bool HasText() const {
        return TextPartCnt > 0;
    }
    size_t DetectDisplayItemCnt(float rowLen) {
        int cnt = MAX_ITEM_CNT;
        if (TextPartLen > rowLen) {
            cnt = 1;
        } else {
            if (ListInfo->HasHeader) {
                cnt--;
            }
            if (HasText()) {
                cnt--;
            }
        }
        cnt = Max(cnt, MIN_ITEM_CNT);
        return cnt;
    }
};

static void FillBestHitSpans(TListSnipCtx& ctx, const TSentsMatchInfo& smInfo) {
    using namespace NStructRearrange;
    TItemInfos infos;
    infos.reserve(ctx.ListInfo->HitItems.size());
    const bool useWeight = ctx.DetectDisplayItemCnt(ctx.RepCtx.LenCfg.GetRowLen()) > 1;
    for(size_t i = 0; i < ctx.ListInfo->HitItems.size(); i++) {
        const int itemID = ctx.ListInfo->HitItems[i];
        const TSpan& hitSpan = ctx.ListInfo->ItemSpans[itemID];
        // fill item seen pos
        TVector<int> itemSeenPos(smInfo.Query.PosCount(), 0);
        for(int j = hitSpan.First; j <= hitSpan.Last; j++) {
            std::pair<int, int> wordRange = GetWordRange(TSpan(j, j), smInfo.SentsInfo);
            if (CheckSpan(wordRange)) {
                const TVector<int>& sentSeenPos = GetSeenPos(wordRange, smInfo.Query, smInfo);
                for (size_t k = 0; k < itemSeenPos.size(); k++) {
                    itemSeenPos[k] += sentSeenPos[k];
                }
            }
        }
        // detect if original item
        bool origItem = false;
        for(size_t j = 0; j < ctx.Items.size(); j++) {
            if (itemID == ctx.Items[j].second) {
                origItem = true;
                break;
            }
        }
        infos.push_back(TItemInfo(ctx.ListID, itemID, &smInfo.Query, itemSeenPos, origItem, useWeight));
    }

    // build sorted
    const TVector<const TItemInfo*>& topItems = BuildSorted(infos, smInfo.Query, MAX_ITEM_CNT);

    // add original hit items
    for (size_t i = 0; i < topItems.size() && ctx.BestHit.ysize() < MAX_ITEM_CNT; i++) {
        if (topItems[i]->OriginalItem) {
            ctx.BestHit.push_back(std::make_pair(topItems[i]->ContainerID, topItems[i]->ItemID));
        }
    }
    // add best hit items
    for(size_t i = 0; i < topItems.size() && ctx.BestHit.ysize() < MAX_ITEM_CNT; i++) {
        if (!topItems[i]->OriginalItem) {
            ctx.BestHit.push_back(std::make_pair(topItems[i]->ContainerID, topItems[i]->ItemID));
        }
    }
}

static const TVector<TSpan> GetItemHitSpans(const TListSnipCtx& ctx) {
    const TItems& items = ctx.Items;
    const NStruct::TListInfo& listInfo = *(ctx.ListInfo.Get());
    TVector<TSpan> spans;

    int max = MAX_ITEM_CNT - listInfo.HitItems.ysize();
    if (listInfo.HasHeader) {
        max--;
    }
    spans.reserve(items.size() + listInfo.HitItems.size() + (listInfo.HasHeader ? 1 : 0) + (max > 0 ? max : 0));

    // insert item spans
    for (size_t i = 0; i < items.size(); i++) {
        spans.push_back(listInfo.ItemSpans[items[i].second]);
    }

    if (max > 0) {
        // post spans
        for(int i = items.back().second + 1; i < static_cast<int>(listInfo.ItemCount) && spans.ysize() <= max; i++) {
            spans.push_back(listInfo.ItemSpans[i]);
        }
        // pre spans
        for (int i = items.front().second - 1; i >= 0 && spans.ysize() <= max; i--) {
            spans.push_back(listInfo.ItemSpans[i]);
        }
    }
    // insert header
    if (listInfo.HasHeader) {
        spans.push_back(listInfo.HeaderSpan);
    }
    // hit items
    for (size_t i = 0; i < ctx.BestHit.size(); i++) {
        spans.push_back(listInfo.ItemSpans[ctx.BestHit[i].second]);
    }

    NSentChecker::Compact(spans);
    return spans;
}

inline static bool InList(const std::pair<int, int>& wordSpan, const TSentsMatchInfo& smInfo, const NStruct::TListInfo& listInfo) {
    TSpan origSpan = GetOrigSentSpan(wordSpan, smInfo.SentsInfo);
    TSpan effectiveSpan(listInfo.Span.First, listInfo.Span.Last);
    if (listInfo.HasHeader) {
        effectiveSpan.First = listInfo.HeaderSpan.First;
    }
    return effectiveSpan.Contains(origSpan);
}

namespace NFeatures {
    enum EZonePos {
        ZP_BEG = 1,
        ZP_END = 2,
        ZP_MID = 4,
        ZP_ALL = 8
    };
    struct TDocFeatures {
        TVector<int> SegSentCount;
        TVector<float> MinSegWeights;

        TDocFeatures(const TArchiveMarkup& markup)
            : SegSentCount(NSegm::STP_COUNT, 0)
            , MinSegWeights(NSegm::STP_COUNT, std::numeric_limits<float>::max())
        {
            const NSegments::TSegmentsInfo* segInfo = markup.GetSegments();
            if (segInfo != nullptr) {
                for(NSegments::TSegmentCIt it = segInfo->SegmentsBegin(); it != segInfo->SegmentsEnd(); ++it) {
                    NSegm::ESegmentType type = segInfo->GetType(it);
                    SegSentCount[type] += it->End.Sent() - it->Begin.Sent();
                    MinSegWeights[type] = Min(MinSegWeights[type], it->Weight);
                }
            }
        }

        bool HasSegment(NSegm::ESegmentType t) const {
            return SegSentCount[t] > 0;
        }
    };

    struct TZoneStat {
        size_t Len = 0;
        ui8 Pos = 0;

        void operator()(const TArchiveSent& sent, TForwardInZoneChecker& zoneIter) {
            const TSentParts parts = IntersectZonesAndSent(zoneIter, sent, true);
            for (const TSentPart& part : parts) {
                if (part.InZone) {
                    Len += part.Part.size();
                    const size_t sentLen = sent.Sent.size();
                    if (part.Part.data() == sent.Sent.data() && part.Part.size() == sentLen) {
                        Pos = ZP_ALL;
                        break;
                    } else if (part.Part.data() == sent.Sent.data() && part.Part.size() < sentLen) {
                        Pos |= ZP_BEG;
                    } else if (part.Part.data() > sent.Sent.data() && part.Part.end() == sent.Sent.end()) {
                        Pos |= ZP_END;
                    } else if (part.Part.data() > sent.Sent.data() && part.Part.end() < sent.Sent.end()) {
                        Pos |= ZP_MID;
                    }
                }
            }
        }
    };

    struct TItemSent {
        const TArchiveSent* Sent;
        NSegm::ESegmentType SegType;
        TZoneStat Dates;

        TItemSent(const TArchiveSent& sent, NSegm::ESegmentType type,
                  TForwardInZoneChecker& dateZoneIter)
            : Sent(&sent), SegType(type), Dates()
        {
            Dates(sent, dateZoneIter);
        }
    };

    class TItemFeatures {
    public:
        TVector<TItemSent> Sents;
        TVector<int> SegSentCount;

        TItemFeatures()
          : SegSentCount(NSegm::STP_COUNT, 0)
        {
        }

        void FeedSent(const TArchiveSent& sent, NSegm::ESegmentType type, TForwardInZoneChecker& dateZoneIter) {
            SegSentCount[type]++;
            Sents.push_back(TItemSent(sent, type, dateZoneIter));
        }

        size_t TextLen() const {
            size_t res = 0;
            for (size_t i = 0; i < Sents.size(); i++) {
                res += Sents[i].Sent->Sent.size();
            }
            return res;
        }
    };
    struct TListFeatures {
        size_t ItemCount = 0;
        int SentBeg = 0;
        int SentEnd = 0;
        bool HasHeader = false;
        bool HeaderLinkInt = false;
        bool HeaderLinkExt = false;
        TVector<TItemFeatures> Items;   // itemId -> item features
        TVector<float> MinSegWeights;   // segType -> min weight
        TVector<int> SegSentCount;      // segType -> sent count

        TListFeatures(const TArchiveMarkup& markup, const TSentsOrder& order, const NStruct::TListInfo& listInfo)
            : Items()
            , MinSegWeights(NSegm::STP_COUNT, std::numeric_limits<float>::max())
            , SegSentCount(NSegm::STP_COUNT, 0)
        {
            ItemCount = listInfo.ItemCount;
            SentBeg = listInfo.Span.First;
            SentEnd = listInfo.Span.Last;
            HasHeader = listInfo.HasHeader;

            Items.resize(ItemCount);
            // collect sent & segment data
            const NSegments::TSegmentsInfo* segInfo = markup.GetSegments();
            TForwardInZoneChecker dateZoneIter(markup.GetArcMarkup(ARC_TEXT).GetZone(AZ_DATER_DATE).Spans);
            int lastSeen = 0;
            TArchiveView v;
            DumpResult(order, v);
            for (size_t i = 0; i < v.Size(); ++i) {
                if (v.Get(i)->SentId >= SentBeg && v.Get(i)->SentId <= SentEnd) {
                    NSegments::TSegmentCIt sit = segInfo->GetArchiveSegment(*v.Get(i));
                    NSegm::ESegmentType type = segInfo->GetType(sit); // safe because have internal validity check
                    SegSentCount[type]++;
                    if (segInfo->IsValid(sit)) {
                        MinSegWeights[type] = Min(MinSegWeights[type], sit->Weight);
                    }

                    if (v.Get(i)->SentId > listInfo.ItemSpans[lastSeen].Last) {
                        lastSeen++;
                    }
                    Items[lastSeen].FeedSent(*v.Get(i), type, dateZoneIter);
                }
            }

            // fill header link info
            HeaderLinkExt = CheckLinkInHeader(listInfo, markup.GetArcMarkup(ARC_TEXT).GetZone(AZ_ANCHOR));
            HeaderLinkInt = CheckLinkInHeader(listInfo, markup.GetArcMarkup(ARC_TEXT).GetZone(AZ_ANCHORINT));
        }

        bool CheckLinkInHeader(const NStruct::TListInfo& listInfo, const TArchiveZone& anc) const {
            for (size_t i = 0; i < anc.Spans.size(); i++) {
                if (listInfo.HeaderSpan.Contains(anc.Spans[i].SentBeg) &&
                    listInfo.HeaderSpan.Contains(anc.Spans[i].SentEnd))
                {
                    return true;
                }
            }
            return false;
        }

        int SentLen() const {
            return SentEnd - SentBeg + 1;
        }
    };
}

namespace NFilters {

    namespace NFilterSettings {
        const static size_t MinItemCnt = 3;
        const static size_t MaxItemLen = 100;
        const static NSegm::ESegmentType SegFilter[] = { NSegm::STP_HEADER, NSegm::STP_CONTENT };
        const static size_t SegFilterSz = Y_ARRAY_SIZE(SegFilter);
        const static NSegm::ESegmentType ScoreFilter = NSegm::STP_CONTENT;
        const static double MinAbsSegWeight = 0;
    }

    static bool ItemCountFlt(const NFeatures::TListFeatures& list) {
        return list.ItemCount >= NFilterSettings::MinItemCnt;
    }

    static bool ItemLengthFlt(const NFeatures::TListFeatures& list) {
        for (int i = 0; i < list.Items.ysize(); i++) {
            if (list.Items[i].TextLen() > NFilterSettings::MaxItemLen) {
                return false;
            }
        }
        return true;
    }

    static bool SegmentFlt(const NFeatures::TListFeatures& list) {
        int sentCnt = 0;
        for(size_t i = 0; i < NFilterSettings::SegFilterSz; i++) {
            sentCnt += list.SegSentCount[NFilterSettings::SegFilter[i]];
        }
        bool allSents = sentCnt == list.SentLen();
        return allSents;
    }

    static bool SegScoreFlt(const NFeatures::TDocFeatures& doc, const NFeatures::TListFeatures& list) {
        NSegm::ESegmentType ctype = NFilterSettings::ScoreFilter;
        if (!doc.HasSegment(ctype) && ctype == NSegm::STP_HEADER) {
            ctype = NSegm::STP_CONTENT;
        }
        float mweight = doc.MinSegWeights[ctype];
        bool all = true;
        for (size_t i = 0; i < list.MinSegWeights.size(); i++) {
            if (list.MinSegWeights[i] < mweight) {
                all = false;
                break;
            }
        }
        return all;
    }

    static bool CommentLikeFlt(const NFeatures::TListFeatures& list) {
        TVector<int> distSet;
        distSet.reserve(list.Items.size());
        int lastSeen = -1;
        // detect dates on edges of list items:
        for (size_t i = 0; i < list.Items.size(); i++) {
            // first sent stat
            // has date at start of first sent
            bool beg = list.Items[i].Sents.front().Dates.Pos & NFeatures::ZP_BEG;
            // first sent is date only
            bool fall = list.Items[i].Sents.front().Dates.Pos & NFeatures::ZP_ALL;
            // first sent len
            size_t fsl = list.Items[i].Sents.front().Sent->Sent.size();
            // date content len in first sent
            size_t fdl = list.Items[i].Sents.front().Dates.Len;
            // last sent stat
            // has date at end of last sent
            bool end = list.Items[i].Sents.back().Dates.Pos & NFeatures::ZP_END;
            // last sent is date only
            bool eall = list.Items[i].Sents.back().Dates.Pos & NFeatures::ZP_ALL;
            // last sent len
            size_t esl = list.Items[i].Sents.back().Sent->Sent.size();
            // date content len in last sent
            size_t edl = list.Items[i].Sents.back().Dates.Len;
            // sent count
            int cnt = list.Items[i].Sents.size();

            // detect date at edge of list item
            // - at start of first sent of list item or at end of last sent of list item
            bool edgeDate = beg || end;
            // - if item has several sents - check if first or last sent contents is date
            edgeDate |= cnt > 1 && fall && (fsl - fdl) < 2;
            edgeDate |= cnt > 1 && eall && (esl - edl) < 2;

            // fill distance between list items with edge dates
            if (edgeDate) {
                if (lastSeen >= 0) {
                    distSet.push_back(i - lastSeen);
                }
                lastSeen = i;
            }
        }

        // detect if distance between items with dates are equal in list
        if (!distSet.empty()) {
            int dist = distSet.front();
            bool eq = true;
            for (size_t i = 0; i < distSet.size(); i++) {
                if (dist != distSet[i]) {
                    eq = false;
                    break;
                }
            }
            // if distance is equal and >1 - probably it's list with comments
            if (eq && distSet.size() > 1 && dist > 1) {
                return false;
            }
        }
        return true;
    }

    static bool MenuLikeFlt(const NFeatures::TListFeatures& list) {
        return !(list.HasHeader && (list.HeaderLinkExt || list.HeaderLinkInt));
    }

    static bool SegAbsWeightFlt(const NFeatures::TListFeatures& list) {
        return list.MinSegWeights[NSegm::STP_CONTENT] >= NFilterSettings::MinAbsSegWeight;
    }

    static bool ListFilter(const NFeatures::TDocFeatures& doc, const NFeatures::TListFeatures& list) {
        return ItemCountFlt(list) &&
               ItemLengthFlt(list) &&
               SegmentFlt(list) &&
               SegScoreFlt(doc, list) &&
               SegAbsWeightFlt(list) &&
               MenuLikeFlt(list) &&
               CommentLikeFlt(list);
    }
}

namespace NItemRearrange {
    using namespace ::NSnippets::NStructRearrange;

    static bool ContainsItem(const TItems& items, const NStruct::TLICoord item) {
        for (size_t i = 0; i < items.size(); i++) {
            if (items[i].first == item.first && items[i].second == item.second) {
                return true;
            }
        }
        return false;
    }

    static void RearrangeCtxItems(TListSnipCtx& ctx, const TReplaceContext& repCtx) {
        const TItems& items = ctx.Items;
        size_t newSz = ctx.DetectDisplayItemCnt(repCtx.LenCfg.GetRowLen());
        TItems res;
        res.reserve(newSz);

        // add list hit items
        if (res.size() < newSz) {
            for (size_t i = 0; i < ctx.BestHit.size() && res.size() < newSz; i++) {
                if (!ContainsItem(res, ctx.BestHit[i])) {
                    res.push_back(ctx.BestHit[i]);
                }
            }
        }

        // adjust size
        size_t adjMin = newSz;
        if (res.size() < (newSz - 1)) {
            adjMin = res.size() + 1;
        }

        //add original hitless items
        if (res.size() < adjMin) {
            for (size_t i = 0; i < items.size() && res.size() < adjMin; i++) {
                if (!ContainsItem(res, items[i])) {
                    res.push_back(items[i]);
                }
            }
        }

        // add list hitless items
        if (res.size() < adjMin) {
            Sort(res.begin(), res.end(), TItemOrderCmp<NStruct::TLICoord>());
            int last = res.back().second;
            for (int i = last + 1; i < static_cast<int>(ctx.ListInfo->ItemCount) && res.size() < adjMin; i++) {
                res.push_back(std::make_pair(ctx.ListID, i));
            }
            int first = res.front().second;
            for (int i = first - 1; i >= 0 && res.size() < adjMin; i--) {
                res.push_back(std::make_pair(ctx.ListID, i));
            }
        }
        Sort(res.begin(), res.end(), TItemOrderCmp<NStruct::TLICoord>());
        ctx.Items.swap(res);
    }
}

static void FillListCtx(const TListArcViewer& arcViewer, const TArchiveMarkup& markup,
                       const TSnip::TSnips& snips,
                       TListSnipCtx& ctx)
{
    TItems resItems;
    int num = 0;
    for(TSnip::TSnips::const_iterator it = snips.begin(); it != snips.end(); ++it, num++) {
        TSpan span = GetOrigSentSpan(it->GetWordRange(), it->GetSentsMatchInfo()->SentsInfo);
        std::pair<NStruct::TLICoord, NStruct::TLICoord> itemRange = arcViewer.CheckList(span);
        if (itemRange.first.first == -1) {
            ctx.TextPartStart = num;
            ctx.TextWordRange = it->GetWordRange();
            ctx.TextPartCnt++;
            if (ctx.TextPartCnt == 1) {
                ctx.TextPartLen = ctx.RepCtx.SnipWordSpanLen.CalcLength(*it);
            }
        } else {
            // filter too short fragment
            if (it->GetLastWord() - it->GetFirstWord() < 4) {
                return;
            }
            // if good len - add items
            for(int i = itemRange.first.second; i <= itemRange.second.second; i++) {
                resItems.push_back(std::make_pair(itemRange.first.first, i));
            }
        }
    }

    // ban many text parts or empty result
    if (ctx.TextPartCnt > 1 || resItems.empty()) {
        return;
    }
    Sort(resItems.begin(), resItems.end(), NStructRearrange::TItemOrderCmp<NStruct::TLICoord>());
    // check different lists
    if (resItems.front().first != resItems.back().first) {
        return;
    }
    // merge equal
    resItems.erase(Unique(resItems.begin(), resItems.end()), resItems.end());

    int listID = resItems.front().first;
    // ban long text snippets
    if (ctx.TextPartCnt == 1) {
        if (ctx.TextPartLen > 2 * ctx.RepCtx.LenCfg.GetRowLen() && arcViewer.HasListHeader(listID)) {
            return;
        }
        if (ctx.TextPartLen > 3 * ctx.RepCtx.LenCfg.GetRowLen()) {
            return;
        }
        if (ctx.TextPartLen > ctx.RepCtx.LenCfg.GetRowLen() && resItems.size() > 2) {
            return;
        }
    }

    // run filters
    arcViewer.FillListInfo(listID, ctx.ListInfo);
    NFeatures::TListFeatures listFeatures(markup, arcViewer.GetSentsOrder(), *(ctx.ListInfo.Get()));
    NFeatures::TDocFeatures docFeatures(markup);
    if (NFilters::ListFilter(docFeatures, listFeatures)) {
        ctx.Items = resItems;
        ctx.ListID = listID;
    }
}

struct TListSnipSpans {
    bool Header = false;
    bool Valid = false;
    TVector<std::pair<int, int>> WordSpans;
};

static TListSnipSpans PrepareSpans(const TListSnipCtx& ctx,
                                   const TSentsMatchInfo& smInfo,
                                   const TSnip& snip,
                                   const TSnipTitle* title)
{
    const TItems& items = ctx.Items;
    const NStruct::TListInfo& listInfo = *(ctx.ListInfo.Get());
    NItemCut::TItemCut itemCut(ctx.RepCtx, ctx.RepCtx.LenCfg.GetRowLen());
    TListSnipSpans res;

    std::pair<int, int> headerSpan = {-1, -1};
    if (listInfo.HasHeader) {
        std::pair<int, int> headerWordRange = GetWordRange(listInfo.HeaderSpan, smInfo.SentsInfo);
        if (CheckSpan(headerWordRange)) {
            std::pair<int, int> cuttedRange = itemCut.Cut(headerWordRange, smInfo, title);
            if (CheckSpan(cuttedRange)) {
                headerSpan = cuttedRange;
                if (!(ctx.RepCtx.Cfg.DropListHeader() && smInfo.MatchesInRange(headerSpan.first, headerSpan.second) == 0)) {
                    res.WordSpans.push_back(cuttedRange);
                    res.Header = true;
                }
            }
        }
    }
    TWordStat wordStatAll(smInfo.Query, smInfo);
    TWordStat wordStat(smInfo.Query, smInfo);
    for (size_t i = 0; i < items.size(); i++) {
        TSpan itemSpan = listInfo.ItemSpans[items[i].second];
        std::pair<int, int> itemWordRange = GetWordRange(itemSpan, smInfo.SentsInfo);
        if (CheckSpan(itemWordRange)) {
            std::pair<int, int> cuttedRange = itemCut.Cut(itemWordRange, smInfo, title);
            if (CheckSpan(cuttedRange)) {
                if (CheckSpan(headerSpan)) {
                    wordStat.SetSpan(headerSpan.first, headerSpan.second);
                    wordStat.AddSpan(cuttedRange.first, cuttedRange.second);
                    if (wordStat.Data().RepeatedWordsScore > 0) {
                        return res;
                    }
                }
                wordStatAll.AddSpan(cuttedRange.first, cuttedRange.second);
                res.WordSpans.push_back(cuttedRange);
            }
        } else {
            return res;
        }
    }
    // check repetitions and matches
    const TWordStatData& wdata = wordStatAll.Data();
    if (wdata.RepeatedWordsScore > 0 || !wdata.HasAnyMatches()) {
        return res;
    }

    // check intersections
    if (ctx.TextPartCnt == 1) {
        TSpan text = GetOrigSentSpan(ctx.TextWordRange, snip.Snips.front().GetSentsMatchInfo()->SentsInfo);
        for(size_t i = 0; i < res.WordSpans.size(); i++) {
            TSpan item = GetOrigSentSpan(res.WordSpans[i], smInfo.SentsInfo);
            if (item.Intersects(text)) {
                return res;
            }
        }
    }

    if (!listInfo.HasHeader && ctx.TextPartStart == 0 && items.size() == 1) {
        return res;
    }

    res.Valid = true;
    return res;
}

static bool UpdateSnips(const TListSnipCtx& ctx, TSnip& snip, const TSentsMatchInfo& smInfo, const TSnipTitle* title,
                        TListSnipData& snipData)
{
    const NStruct::TListInfo& listInfo = *(ctx.ListInfo.Get());
    TSnip::TSnips::iterator it = snip.Snips.begin();
    snipData.ItemCount = listInfo.ItemCount;

    const TListSnipSpans& listSpans = PrepareSpans(ctx, smInfo, snip, title);
    if (!listSpans.Valid) {
        return false;
    }

    int n = 0;
    bool changed = false;
    TSnip::TSnips snips;
    while(it != snip.Snips.end()) {
        if (InList(it->GetWordRange(), *it->GetSentsMatchInfo(), listInfo)) {
            if (!changed) {
                if (listSpans.Header) {
                    snipData.ListHeader = n;
                    snipData.First = n + 1;
                } else {
                    snipData.First = n;
                }
                for(size_t i = 0; i < listSpans.WordSpans.size(); i++) {
                    snips.push_back(TSingleSnip(listSpans.WordSpans[i], smInfo));
                    n++;
                }
                snipData.Last = n - 1;
            }
            changed = true;
            ++it;
            continue;
        } else {
            const float maxRowLen = 0.95f * ctx.RepCtx.LenCfg.GetRowLen();
            NItemCut::TItemCut itemCut(ctx.RepCtx, maxRowLen);
            const TSentsMatchInfo& itemSMInfo = *it->GetSentsMatchInfo();
            std::pair<int, int> wordSpan = itemCut.Cut(it->GetWordRange(), itemSMInfo, title);
            if (CheckSpan(wordSpan)) {
                snips.push_back(TSingleSnip(wordSpan, itemSMInfo));
                snips.back().SetAllowInnerDots(true);
            }
        }
        n++;
        ++it;
    }
    if (changed) {
        snip.Snips = snips;
        return true;
    }

    return false;
}

void TSnipListReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& repCtx = manager->GetContext();
    if (repCtx.IsByLink ||
        !ArcViewer.HasLists() ||
        repCtx.Snip.Snips.empty())
    {
        return;
    }

    TListSnipCtx listCtx(repCtx);
    FillListCtx(ArcViewer, repCtx.Markup, repCtx.Snip.Snips, listCtx);

    if (listCtx.ListID == -1) {
        return;
    }

    FillBestHitSpans(listCtx, *repCtx.Snip.Snips.front().GetSentsMatchInfo());
    const TVector<TSpan>& s = GetItemHitSpans(listCtx);
    TRetainedSentsMatchInfo& customSnip = manager->GetCustomSnippets().CreateRetainedInfo();
    BuildSMInfo(NSentChecker::TSpanChecker(s), customSnip, repCtx, ArcViewer.GetSentsOrder());

    NItemRearrange::RearrangeCtxItems(listCtx, repCtx);

    TListSnipData listData;
    TSnip newSnip = repCtx.Snip;

    bool check = UpdateSnips(listCtx, newSnip, *customSnip.GetSentsMatchInfo(), &repCtx.SuperNaturalTitle, listData);

    if (check) {
        double worig = TSimpleSnipCmp::CalcWeight(repCtx.Snip, nullptr, true);
        double wlist = TSimpleSnipCmp::CalcWeight(newSnip, nullptr, true);

        if (worig > wlist) {
            return;
        }

        // drop snippet if list header is not the first passage
        if (listData.ListHeader > 0) {
            return;
        }

        TReplaceResult result;
        result.UseSnip(newSnip, LIST_SNIPPETS);
        result.AppendSpecSnipAttr(LIST_DATA, listData.Dump(repCtx.Cfg.DropListStat()));
        manager->Commit(result, MRK_LIST);

        ISnippetDebugOutputHandler* dbg = manager->GetCallback().GetDebugOutput();
        if (dbg) {
            manager->GetCallback().OnBestFinal(newSnip, false);
        }
    }
}

}
