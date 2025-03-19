#include "struct_common.h"

#include <kernel/snippets/algo/maxfit.h>
#include <kernel/snippets/algo/one_span.h>

#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/hits/topn.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/titles/make_title/util_title.h>
#include <kernel/snippets/uni_span_iter/uni_span_iter.h>
#include <kernel/snippets/weight/weighter.h>

namespace NSnippets {

bool CheckSpan(const std::pair<int, int> wordSpan) {
    return wordSpan.first > -1 && wordSpan.second > -1;
}

TVector<int> GetSeenPos(const std::pair<int, int> wordRange, const TQueryy& q, const TSentsMatchInfo& smInfo) {
    TVector<int> seenPos(q.PosCount(), 0);
    for (int w = wordRange.first; w <= wordRange.second; w++) {
        for (int lemmaId : smInfo.GetNotExactMatchedLemmaIds(w)) {
            for (int pos : q.Id2Poss[lemmaId]) {
                seenPos[pos]++;
            }
        }
    }
    return seenPos;
}

static inline bool CheckSpan(const TSpan span) {
    return span.First > -1 && span.Last > -1;
}

TSpan GetOrigSentSpan(const std::pair<int, int> wordRange, const TSentsInfo& sentsInfo) {
    TSpan res(-1, -1);
    int startSent = sentsInfo.WordId2SentId(wordRange.first);
    int stopSent = sentsInfo.WordId2SentId(wordRange.second);
    res.First = sentsInfo.GetOrigSentId(startSent);
    res.Last = sentsInfo.GetOrigSentId(stopSent);
    return res;
}

std::pair<int, int> GetWordRange(TSpan span, const TSentsInfo& sentsInfo) {
    if (!CheckSpan(span)) {
        return {-1, -1};
    }
    int begSent = sentsInfo.TextArchIdToSentIds(span.First).first;
    int endSent = sentsInfo.TextArchIdToSentIds(span.Last).second;
    if (begSent >= 0 && endSent >= 0) {
        int begWord = sentsInfo.FirstWordIdInSent(begSent);
        int endWord = sentsInfo.LastWordIdInSent(endSent);
        return {begWord, endWord};
    } else {
        return {-1, -1};
    }
}

void BuildSMInfo(const NSentChecker::TSpanChecker& checker,
                 TRetainedSentsMatchInfo& rsmInfo,
                 const TReplaceContext& repCtx,
                 const TSentsOrder& sentsOrder)
{
    TArchiveView preview;
    TArchiveView view;
    DumpResult(sentsOrder, preview);
    for (size_t i = 0; i < preview.Size(); ++i) {
        if (!preview.Get(i)->Sent.empty() && checker(preview.Get(i)->SentId)) {
            view.PushBack(preview.Get(i));
        }
    }
    rsmInfo.SetView(&repCtx.Markup, view, TRetainedSentsMatchInfo::TParams(repCtx.Cfg, repCtx.QueryCtx));
}

namespace NSentChecker {
    void Compact(TVector<TSpan>& spans) {
        Sort(spans.begin(), spans.end());
        const size_t cnt = spans.size();
        size_t pre = 0;
        size_t cur = 1;

        while(cur < cnt) {
            while(cur < cnt && spans[cur].First == -1) {
                cur++;
            }
            if (cur >= cnt) {
                break;
            }
            if (cur - pre > 1) {
                spans[pre + 1] = spans[cur];
                spans[cur].First = spans[cur].Last = -1;
                cur = pre + 1;
            }
            if (spans[cur].Last <= spans[pre].Last) {
                spans[cur].First = spans[cur].Last = -1;
            }
            if (spans[cur].First == spans[pre].Last + 1) {
                spans[pre].Last = spans[cur].Last;
                spans[cur].First = spans[cur].Last = -1;
            }
            if (spans[cur].First != -1) {
                pre++; cur = pre + 1;
            }
        }
        spans.resize(pre + 1);
    }
}

namespace NStructRearrange {
    void TItemInfo::CalcWeight(bool useWeight) {
        if (!QCtx) {
            return;
        }
        CachedWeight = 0;
        const TVector<double>& w = QCtx->PosSqueezer->GetSqueezedWeights();
        const TVector<bool>& s = QCtx->PosSqueezer->Squeeze(SeenLikePos);
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i]) {
                useWeight ? CachedWeight += w[i] : CachedWeight++;
            }
        }
    }

    struct TItemInfoHash {
        hash<std::pair<int, int>> Impl;
        size_t operator()(const TItemInfo* item) const
        {
            return Impl(std::make_pair(item->ContainerID, item->ItemID));
        }
    };

    struct TItemInfoEq {
        bool operator()(const TItemInfo* const i1, const TItemInfo* const i2) const {
            return i1->ContainerID == i2->ContainerID && i1->ItemID == i2->ItemID;
        }
    };

    TVector<const TItemInfo*> BuildSorted(const TItemInfos& infos, const TQueryy& qctx, int minItems) {
        typedef TMultiTop<const TItemInfo*, TItemWeightCmp, double, TItemInfoHash, TItemInfoEq> TMT;
        const TVector<double>& weights = qctx.PosSqueezer->GetSqueezedWeights();
        TMT top(minItems, weights, false);
        int n = 0;
        for (TItemInfos::const_iterator it = infos.begin(); it != infos.end(); ++it, n++) {
            const TItemInfo* i = &(*it);
            top.PushVector(qctx.PosSqueezer->Squeeze(i->SeenLikePos), i);
        }
        return top.ToSortedVector();
    }
}

namespace NItemCut {
    TItemCut::TItemCut(const TReplaceContext& repCtx, float itemLength)
        : RepCtx(repCtx)
        , ItemLength(itemLength)
    {
    }

    std::pair<int, int> TItemCut::Cut(const std::pair<int, int>& wordRange,
        const TSentsMatchInfo& sentsMatchInfo, const TSnipTitle* title)
    {
        TNoRestr restr;
        int startSent = sentsMatchInfo.SentsInfo.WordId2SentId(wordRange.first);
        int stopSent = sentsMatchInfo.SentsInfo.WordId2SentId(wordRange.second);
        TUniSpanIter wordSpan(sentsMatchInfo, restr, restr, RepCtx.SnipWordSpanLen, startSent, stopSent);
        TMxNetWeighter w(sentsMatchInfo, RepCtx.Cfg, RepCtx.SnipWordSpanLen, title, RepCtx.LenCfg.GetMaxSnipLen());

        TSnip snip = NSnipWordSpans::GetTSnippet(
            w, wordSpan, sentsMatchInfo, ItemLength, CS_TEXT_ARC, nullptr, nullptr, nullptr);
        if (!snip.Snips) {
            snip = NMaxFit::GetTSnippet(RepCtx.Cfg, RepCtx.SnipWordSpanLen, TSingleSnip(wordRange, sentsMatchInfo), ItemLength);
            if (!snip.Snips) {
                return {-1, -1};
            }
        }
        return snip.Snips.front().GetWordRange();
    }
}

}
