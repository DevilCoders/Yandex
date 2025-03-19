#pragma once

#include <kernel/snippets/span/span.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <utility>

namespace NSnippets {

class TSentsInfo;
class TSentsMatchInfo;
class TReplaceContext;
class TQueryy;
class TRetainedSentsMatchInfo;
class TSentsOrder;
class TSnipTitle;

bool CheckSpan(const std::pair<int, int> wordSpan);
TVector<int> GetSeenPos(const std::pair<int, int> wordRange, const TQueryy& q, const TSentsMatchInfo& smInfo);
TSpan GetOrigSentSpan(const std::pair<int, int> wordRange, const TSentsInfo& sentsInfo);
std::pair<int, int> GetWordRange(const TSpan span, const TSentsInfo& sentsInfo);

namespace NSentChecker {
    void Compact(TVector<TSpan>& spans);
    struct TSpanChecker {
    private:
        mutable size_t CurrentSpan;
        const TVector<TSpan>& Spans;

    public:
        TSpanChecker(const TVector<TSpan>& spans) : CurrentSpan(0), Spans(spans) {};

        bool operator()(int sentId) const
        {
            if (CurrentSpan >= Spans.size()) {
                return false;
            }
            if (sentId < Spans[CurrentSpan].First) {
                return false;
            }
            if (sentId <= Spans[CurrentSpan].Last) {
                if (Spans[CurrentSpan].Last == sentId) {
                    CurrentSpan++;
                }
                return true;
            }
            return false;
        }
    };
}

void BuildSMInfo(const NSentChecker::TSpanChecker& checker,
                 TRetainedSentsMatchInfo& rsmInfo,
                 const TReplaceContext& repCtx,
                 const TSentsOrder& sentsOrder);

namespace NStructRearrange {
    struct TItemInfo {
        const TQueryy* QCtx;
        double CachedWeight;
        int ContainerID;
        int ItemID;
        bool OriginalItem;
        TVector<int> SeenLikePos;

        TItemInfo(int contID, int itemID, const TQueryy* qctx, const TVector<int>& seenPos, bool origItem, bool useWeight)
            : QCtx(qctx)
            , CachedWeight(0)
            , ContainerID(contID)
            , ItemID(itemID)
            , OriginalItem(origItem)
            , SeenLikePos(seenPos)
        {
            CalcWeight(useWeight);
        }

        double Weight() const {
            return CachedWeight;
        }
        void CalcWeight(bool useWeight);
    };
    typedef TVector<TItemInfo> TItemInfos;
    TVector<const TItemInfo*> BuildSorted(const TItemInfos& infos, const TQueryy& qctx, int minItems);

    template<class TCoord>
    struct TItemOrderCmp {
        bool operator()(const TCoord i1, const TCoord i2) const {
            if (i1.first != i2.first) {
                return i1.first < i2.first;
            } else {
                return i1.second < i2.second;
            }
        }
    };

    struct TItemWeightCmp {
        bool operator()(const TItemInfo* i1, const TItemInfo* i2) const {
            double w1 = i1->Weight();
            double w2 = i2->Weight();
            if (Abs(w1 - w2) < 1e-6) {
                if (i1->ContainerID == i2->ContainerID) {
                    return i1->ItemID < i2->ItemID;
                } else {
                    return i1->ContainerID < i2->ContainerID;
                }
            }
            return w1 > w2;
        }
        bool operator()(const TItemInfo& i1, const TItemInfo& i2) const {
            return operator()(&i1, &i2);
        }
    };
}

namespace NItemCut {
    class TItemCut {
    private:
        const TReplaceContext& RepCtx;
        float ItemLength = 0.0f;
    public:
        TItemCut(const TReplaceContext& repCtx, float itemLength);

        std::pair<int, int> Cut(const std::pair<int, int>& wordRange,
            const TSentsMatchInfo& sentsMatchInfo, const TSnipTitle* title);
    };
}

}
