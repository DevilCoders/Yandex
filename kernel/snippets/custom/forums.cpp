#include "forums.h"

#include <kernel/snippets/algo/maxfit.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/snip_builder/snip_builder.h>
#include <kernel/snippets/smartcut/cutparam.h>
#include <kernel/snippets/titles/make_title/util_title.h>
#include <kernel/snippets/weight/weighter.h>

#include <util/charset/wide.h>
#include <util/charset/unidata.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/vector.h>

#include <array>
#include <utility>

namespace NSnippets
{

namespace {

static const float FORUM_MESSAGES_WIDTH = 0.81f;
static const float FORUM_NAME_LEN = 0.6f;
static const float FORUM_DESCR_LEN = 1.0f;
static const float META_DESCR_LEN = 1.0f;
static const int SUBITEMS_TO_DISPLAY = 2;
static const double DESCR_WEIGHT_FACTOR = 0.1;


class TWorkImpl
{
    struct TForumSnipPart
    {
        const TForumMessageZone& Message;
        TSnip Snip;
        bool Clickable = true;

        explicit TForumSnipPart(const TForumMessageZone& message)
            : Message(message)
        {
        }
    };

    typedef TVector<TForumSnipPart> TForumSnipParts;

    const TReplaceContext& Context;
    const TForumMarkupViewer& ArcViewer;
    bool EvalAfterCutting = false;
    bool WeighBySnipFormula = false;
    bool WeighByPopularity = false;

    void GrowSnip(TSnip& snip, float toLength, const TWordSpanLen& wordSpanLen)
    {
        if (snip.Snips.empty()) {
            return;
        }
        const TSentsMatchInfo& smi = *snip.Snips.front().GetSentsMatchInfo();
        const TSentsInfo& si = smi.SentsInfo;
        TSnipBuilder sb(smi, wordSpanLen, toLength, toLength);
        for (const TSingleSnip& ssnip : snip.Snips) {
            if (!sb.Add(si.WordId2SentWord(ssnip.GetFirstWord()), si.WordId2SentWord(ssnip.GetLastWord()))) {
                return;
            }
        }
        sb.GrowLeftToSent();
        sb.GrowRightToSent();
        while (sb.GrowLeftWordInSent()) {
            continue;
        }
        while (sb.GrowRightWordInSent()) {
            continue;
        }
        sb.GlueTornSents();
        snip = sb.Get(InvalidWeight);
    }

    TForumSnipParts PartitionSnippetByMessage()
    {
        TForumSnipParts result;
        result.reserve(5);

        TVector<TForumMessageZone>::const_iterator msgIter = ArcViewer.ForumZones.begin(), msgEnd = ArcViewer.ForumZones.end();
        TVector<TForumMessageZone>::const_iterator prevMsg = msgEnd;

        for (const TSingleSnip& ssnip : Context.Snip.Snips) {
            int sentBeg = ssnip.GetSentsMatchInfo()->SentsInfo.WordId2SentId(ssnip.GetFirstWord());
            int arcSentBeg = ssnip.GetSentsMatchInfo()->SentsInfo.GetOrigSentId(sentBeg);
            while (msgIter != msgEnd && msgIter->Span.SentEnd < arcSentBeg) {
                ++msgIter;
            }
            if (msgIter != msgEnd && msgIter->Span.SentBeg <= arcSentBeg) {
                if (prevMsg != msgIter) {
                    result.push_back(TForumSnipPart(*msgIter));
                }
                result.back().Snip.Snips.push_back(ssnip);
                prevMsg = msgIter;
            }
        }
        return result;
    }

    void MakeStaticSnip(TForumSnipParts& parts, TCustomSnippetsStorage& customStorage, const TWordSpanLen& wordSpanLen)
    {
        const float rowLen = Context.LenCfg.GetRowLen();
        int remainingRows = 2;

        for (const TForumMessageZone& zone : ArcViewer.ForumZones) {
            if (remainingRows <= 0) {
                break;
            }
            TRetainedSentsMatchInfo& retainedInfo = customStorage.CreateRetainedInfo();
            retainedInfo.SetView(&Context.Markup, zone.Sents, TRetainedSentsMatchInfo::TParams(Context.Cfg, Context.QueryCtx));
            const TSentsMatchInfo& smi = *retainedInfo.GetSentsMatchInfo();
            if (smi.WordsCount() == 0) {
                continue;
            }

            TSingleSnip message(0, smi.WordsCount() - 1, smi);
            TSnip res = NMaxFit::GetTSnippet(Context.Cfg, wordSpanLen, message, remainingRows * rowLen);
            if (!res.Snips) {
                continue;
            }
            TSingleSnip fragment = res.Snips.front();

            float actualLength = wordSpanLen.CalcLength(fragment);
            while (actualLength > 0.0f) {
                actualLength -= rowLen;
                --remainingRows;
            }

            parts.push_back(TForumSnipPart(zone));
            parts.back().Snip.Snips.push_back(fragment);
            parts.back().Clickable = false;
        }
    }

public:
    TWorkImpl(const TForumMarkupViewer& arcViewer, const TReplaceContext& context)
        : Context(context)
        , ArcViewer(arcViewer)
    {
    }

    void DoReplaceWithMessages(TReplaceManager* manager)
    {
        TForumSnipParts parts = PartitionSnippetByMessage();

        const TWordSpanLen* messagesWordSpanLen = &Context.SnipWordSpanLen;
        THolder<TWordSpanLen> messagesWordSpanLenHolder;
        if (Context.Cfg.GetSnipCutMethod() == TCM_PIXEL) {
            int pixelsInLine = static_cast<int>(Context.Cfg.GetYandexWidth() * FORUM_MESSAGES_WIDTH);
            TCutParams cutParams = TCutParams::Pixel(pixelsInLine, Context.Cfg.GetSnipFontSize());
            messagesWordSpanLenHolder.Reset(new TWordSpanLen(cutParams));
            messagesWordSpanLen = messagesWordSpanLenHolder.Get();

            const float maxLen = parts.size() == 1 ? 3.0 : (parts.size() == 2 ? 2.0 : 1.0);
            for (TForumSnipPart& part : parts) {
                GrowSnip(part.Snip, maxLen, *messagesWordSpanLen);
            }
        }

        if (parts.empty()) {
            MakeStaticSnip(parts, manager->GetCustomSnippets(), *messagesWordSpanLen);
        }

        TReplaceResult result;
        TSnip forumSnip;

        size_t partIndex = 0;
        size_t partsCount = parts.size();
        for (TForumSnipPart& part : parts) {
            ++partIndex;
            if (part.Snip.Snips.empty()) {
                continue;
            }
            TSingleSnip& firstAdded = part.Snip.Snips.front();
            TSingleSnip& lastAdded = part.Snip.Snips.back();
            forumSnip.Snips.splice(forumSnip.Snips.end(), part.Snip.Snips);
            firstAdded.AddPassageAttr("forum_date", WideToUTF8(part.Message.Date));
            if (part.Clickable) {
                firstAdded.AddPassageAttr("forum_anchor", WideToUTF8(part.Message.Anchor));
            }
            if (partIndex > 1 && firstAdded.BeginsWithSentBreak()) {
                firstAdded.AddPassageAttr("forum_lead_ell", "1");
            }
            if (partIndex < partsCount && lastAdded.EndsWithSentBreak()) {
                lastAdded.AddPassageAttr("forum_trail_ell", "1");
            }
        }
        for (TSingleSnip& ss : forumSnip.Snips) {
            ss.AddPassageAttr("forum_frag", "1");
        }

        result.UseSnip(forumSnip, "forums");

        if (ArcViewer.ForumTitle) {
            TSnipTitle forumTitle = MakeSpecialTitle(ArcViewer.ForumTitle, Context.Cfg, Context.QueryCtx);
            if (forumTitle.GetTitleString().length() > 10 &&
                TSimpleSnipCmp(Context.QueryCtx, Context.Cfg).Add(Context.NaturalTitle) <= TSimpleSnipCmp(Context.QueryCtx, Context.Cfg).Add(forumTitle))
            {
                result.UseTitle(forumTitle);
            }
        }

        if (ArcViewer.PageNum > 0) {
            result.AppendSpecSnipAttr("forum_page", ToString<int>(ArcViewer.PageNum));
        }
        if (ArcViewer.NumPages > 0) {
            result.AppendSpecSnipAttr("forum_total_pages", ToString<int>(ArcViewer.NumPages));
        }
        if (ArcViewer.NumItems > 0) {
            result.AppendSpecSnipAttr("forum_total_messages", ToString<int>(ArcViewer.NumItems));
            if (ArcViewer.NumItemsPrecise)
                result.AppendSpecSnipAttr("forum_messages_precise", "1");
        }

        manager->Commit(result, MRK_FORUM);
    }

    struct TTopEntry {
        std::array<double, 3> Precedence;
        const TForumMessageZone* Zone = nullptr;
        TSnip Snip;
        const TSentsMatchInfo* SentsMatchInfo = nullptr;

        TTopEntry() {
            Precedence.fill(0.0);
        }

        bool operator == (const TTopEntry& other) const {
            return this->Zone == other.Zone;
        }

        bool IsValid() const {
            return !Snip.Snips.empty() && Snip.Weight != INVALID_SNIP_WEIGHT;
        }
    };

    struct TTopEntryCmp
    {
        bool operator() (const TTopEntry& left, const TTopEntry& right) const {
            for (size_t i = 0; i < left.Precedence.size(); ++i) {
                double lval = left.Precedence[i];
                double rval = right.Precedence[i];
                if (lval > rval)
                    return true;
                else if (lval < rval)
                    return false;
            }
            return false;
        }
    };

    class TTopNCandidates
    {
        const size_t N;
        TVector<TTopEntry>& C;
        TTopEntryCmp Cmp;
    public:
        TTopNCandidates(TVector<TTopEntry>& c, size_t n) : N(n), C(c) {
            C.clear();
            C.reserve(N + 1);
        }
        void Push(const TTopEntry& p) {
            C.push_back(p);
            PushHeap(C.begin(), C.end(), Cmp);
            if (C.size() > N) {
                PopHeap(C.begin(), C.end(), Cmp);
                C.pop_back();
            }
        }
    };

    struct TDisplayOrderCmp
    {
        bool operator() (const TTopEntry& left, const TTopEntry& right) const {
            const TForumMessageZone* leftZone = left.Zone;
            const TForumMessageZone* rightZone = right.Zone;
            size_t leftPop = leftZone->Popularity;
            size_t rightPop = rightZone->Popularity;
            if (leftPop == rightPop)
                return leftZone->Position < rightZone->Position;
            else
                return leftPop > rightPop;
        }
    };

    double SimpleSnipWeight(const TSnip& snip)
    {
        TSimpleSnipCmp cmp(Context.QueryCtx, Context.Cfg, true, true);
        cmp.Add(snip);
        double result = cmp.GetWeight();
        if (result < 0.01)
            return .0;
        else
            return result;
    }

    void CalcEntryWeight(TTopEntry& topEntry)
    {
        const TSnip::TSnips& snips = topEntry.Snip.Snips;
        if (snips.empty()) {
            topEntry.Snip.Weight = 0;
        } else if (WeighBySnipFormula) {
            TMxNetWeighter weighter(*topEntry.SentsMatchInfo, Context.Cfg, Context.SnipWordSpanLen, &Context.SuperNaturalTitle, Context.LenCfg.GetMaxSnipLen());
            TVector<std::pair<int, int>> spans;
            for (const TSingleSnip& ss : snips) {
                spans.push_back(ss.GetWordRange());
            }
            weighter.SetSpans(spans);
            topEntry.Snip.Weight = weighter.GetWeight();
        } else {
            double fullWeight = SimpleSnipWeight(topEntry.Snip);
            double headerWeight = SimpleSnipWeight(TSnip(snips.front(), InvalidWeight));
            topEntry.Snip.Weight = headerWeight + (fullWeight - headerWeight) * DESCR_WEIGHT_FACTOR;
            if (WeighByPopularity) {
                topEntry.Snip.Weight = topEntry.Snip.Weight > 0.01 ? 1 : 0;
            }
        }
        topEntry.Precedence[0] = topEntry.Snip.Weight;
        topEntry.Precedence[1] = topEntry.Zone->Popularity;
        topEntry.Precedence[2] = -((double)topEntry.Zone->Position);
    }

    bool SmartCutForumSnip(TSnip& snip)
    {
        bool isFirst = true;
        for (TSingleSnip& ssnip : snip.Snips) {
            float maxLen = (isFirst ? FORUM_NAME_LEN : FORUM_DESCR_LEN) * Context.LenCfg.GetRowLen();
            TSnip res = NMaxFit::GetTSnippet(Context.Cfg, Context.SnipWordSpanLen, ssnip, maxLen);
            if (!res.Snips) {
                return false;
            }
            ssnip.SetWordRange(res.Snips.front().GetFirstWord(), res.Snips.front().GetLastWord());
            isFirst = false;
        }
        return true;
    }

    TTopEntry GetStaticEntry(const TForumMessageZone& zone, TCustomSnippetsStorage& customStorage)
    {
        if (!zone.Content) {
            return TTopEntry();
        }

        TVector<TUtf16String> sents;
        sents.push_back(zone.Content);
        // Long content starting with "(" indicates parsing error
        if (zone.LongContent && zone.LongContent[0] != L'(') {
            sents.push_back(zone.LongContent);
        }
        TRetainedSentsMatchInfo& retainedInfo = customStorage.CreateRetainedInfo();
        retainedInfo.SetView(sents, TRetainedSentsMatchInfo::TParams(Context.Cfg, Context.QueryCtx));

        TTopEntry newEntry;
        newEntry.Zone = &zone;
        newEntry.SentsMatchInfo = retainedInfo.GetSentsMatchInfo();
        newEntry.Snip = retainedInfo.AllAsSnip();

        if (newEntry.Snip.Snips.empty()) {
            return TTopEntry();
        }

        if (!EvalAfterCutting) {
            CalcEntryWeight(newEntry);
        }
        if (!SmartCutForumSnip(newEntry.Snip)) {
            return TTopEntry();
        }
        if (EvalAfterCutting) {
            CalcEntryWeight(newEntry);
        }

        return newEntry;
    }

    void DoReplaceWithForumsOrTopics(TReplaceManager* manager)
    {
        TVector<TTopEntry> top;
        TTopNCandidates topCands(top, SUBITEMS_TO_DISPLAY);

        for (const TForumMessageZone& zone : ArcViewer.ForumZones) {
            TTopEntry newEntry = GetStaticEntry(zone, manager->GetCustomSnippets());
            if (!newEntry.IsValid())
                continue;
            topCands.Push(newEntry);
        }

        Sort(top.begin(), top.end(), TDisplayOrderCmp());
        if (top.empty()) {
            manager->ReplacerDebug("No valid forum entries");
            return;
        }

        TReplaceResult result;
        TSnip snip;

        bool metadataFail = true;

        for (const TTopEntry& entry : top) {
            const TForumMessageZone* zone = entry.Zone;
            if (zone->Date || zone->HasPopularity) {
                metadataFail = false;
            }
            const TSnip& entrySnip = entry.Snip;
            snip.Snips.push_back(entrySnip.Snips.front());

            if (zone->HasPopularity)
                snip.Snips.back().AddPassageAttr("forum_items", ToString(zone->Popularity));
            if (zone->Anchor)
                snip.Snips.back().AddPassageAttr("forum_url", WideToUTF8(zone->Anchor));
            if (zone->Date)
                snip.Snips.back().AddPassageAttr("forum_date", WideToUTF8(zone->Date));

            if (entrySnip.Snips.size() > 1) {
                snip.Snips.push_back(entrySnip.Snips.back());
                snip.Snips.back().AddPassageAttr("forum_longdescr", "1");
            }
        }

        if (metadataFail) {
            manager->ReplacerDebug("Forum items counts/dates are missing");
            return;
        }

        for (TSnip::TSnips::iterator ii = snip.Snips.begin(), end = --snip.Snips.end(); ii != end; ++ii) {
            TSingleSnip& ss = *ii;
            if (ss.EndsWithSentBreak()) {
                ss.AddPassageAttr("forum_trail_ell", "1");
            }
        }

        const char* src = (ArcViewer.PageKind == TForumMarkupViewer::THREADS) ? "forum_topic" : "forum_forums";
        if (Context.MetaDescr.MayUse()) {
            TUtf16String meta = Context.MetaDescr.GetTextCopy();
            TSmartCutOptions options(Context.Cfg);
            options.MaximizeLen = true;
            const float maxLen = META_DESCR_LEN * Context.LenCfg.GetRowLen();
            SmartCut(meta, Context.IH, maxLen, options);
            if (meta) {
                result.UseText(meta, src);
            }
        }
        result.UseSnip(snip, src);

        if (ArcViewer.PageNum > 0) {
            result.AppendSpecSnipAttr("forum_page", ToString<int>(ArcViewer.PageNum));
        }
        if (ArcViewer.NumPages > 0) {
            result.AppendSpecSnipAttr("forum_total_pages", ToString<int>(ArcViewer.NumPages));
        }
        if (ArcViewer.NumItems > 0) {
            result.AppendSpecSnipAttr("forum_items", ToString<int>(ArcViewer.NumItems));
            if (ArcViewer.NumItemsPrecise) {
                result.AppendSpecSnipAttr("forum_items_precise", "1");
            }
        }

        manager->Commit(result, MRK_FORUM);
    }

    bool IsInStupidBlacklist(const TString& url)
    {
        TStringBuf host = CutWWWPrefix(GetOnlyHost(url));
        return (host == "xakep.ru" || host == "bahisci.com" || host == "privorot.club");
    }

    void DoWork(TReplaceManager* manager)
    {
        if (!ArcViewer.IsValid() || Context.IsByLink) {
            return;
        }
        if (IsInStupidBlacklist(Context.Url)) {
            manager->ReplacerDebug("URL is in stupid blacklist");
            return;
        }
        if (ArcViewer.PageKind == TForumMarkupViewer::MESSAGES) {
            DoReplaceWithMessages(manager);
        } else {
            DoReplaceWithForumsOrTopics(manager);
        }
    }
};

}

void TForumReplacer::DoWork(TReplaceManager* manager)
{
    TWorkImpl workImpl(ForumViewer, manager->GetContext());
    workImpl.DoWork(manager);
}

}
