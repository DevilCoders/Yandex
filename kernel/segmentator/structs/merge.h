#pragma once

#include "merge_desiders.h"
#include <kernel/segmentator/structs/structs.h>

namespace NSegm {

//actually, this code in most duplicates it from ../basic_merge.h
template<typename TDesider, typename TListAccessor>
inline bool Merge(typename TListAccessor::TListType& segs) {
    if (TListAccessor::Empty(segs))
        return false;

    bool merged = false;

    typename TListAccessor::TListType::iterator it = TListAccessor::Begin(segs);

    do {
        if (TListAccessor::Front(segs) == TListAccessor::Back(segs))
            return false;

        if (!TDesider::AllowMerge(&*it)) {
            ++it;
            continue;
        }

        for (bool merge = true; merge && TDesider::AllowMerge(&*it);) {
            merge = false;
            TBlockDist pdist, ndist;
            typename TListAccessor::TNode * prev = TListAccessor::GetPrev(it, TListAccessor::Begin(segs));
            typename TListAccessor::TNode * next = TListAccessor::GetNext(it, TListAccessor::End(segs));
            pdist = TListAccessor::GetBlockDist(prev, &*it);
            ndist = TListAccessor::GetBlockDist(&*it, next);

            if (prev && TDesider::AcceptMergePrev(&*it, prev, pdist) && (TDesider::DesideMergePrev(pdist,
                    ndist) || !next)) {
                it->MergePrev(*prev);
                it = TListAccessor::Erase(--it, &segs);
                merged = merge = true;
            } else if (next && TDesider::AcceptMergeNext(&*it, next, ndist) && (TDesider::DesideMergeNext(
                    pdist, ndist) || !prev)) {
                it->MergeNext(*next);
                it = TListAccessor::EraseBack(++it, &segs);
                merged = merge = true;
            }
        }

        ++it;
    } while (it != TListAccessor::End(segs));
    return merged;
}

inline bool EqualSignatures(const TSegmentSpan& a, const TSegmentSpan& b) {
    return a.FirstBlock.Included == b.FirstBlock.Included
                    && a.LastBlock.Included == b.LastBlock.Included
                    && a.FirstSignature == b.FirstSignature
                    && a.LastSignature == b.LastSignature;
}

template<typename TIterator>
ui32 inline GetMaxPeriodicLen(TIterator begin, TIterator end) {
    if (begin == end)
        return 0;

    TIterator reference = begin;
    ui32 l = 1;
    ++begin;

    while (begin != end && EqualSignatures(*begin, *reference)) {
        ++begin;
        ++l;
    }

    return l;
}

template<typename TListAccessor>
inline void PeriodicMerge(typename TListAccessor::TListType& segments) {
    for (typename TListAccessor::TListType::iterator it = TListAccessor::Begin(segments); it != TListAccessor::End(segments); ++it) {
        ui32 periodicLen = GetMaxPeriodicLen<typename TListAccessor::TListType::iterator> (it, TListAccessor::End(segments));

        if (periodicLen < 2)
            continue;

        //iterating the intrusive list and unlinking a portion of it
        typename TListAccessor::TListType::iterator mit = it;
        ++mit;

        for (ui32 i = 0; i < periodicLen - 1; ++i) {
            it->MergeNext(*mit);
            mit = TListAccessor::Erase(mit, &segments);
        }
    }
}


template <bool main>
TSegmentSpans MakeCoarseSpans(TSegmentSpans spans) {
    bool merged = true;

    while (merged) {
        merged = Merge<THeaderToNodeMergeDesider<4, BST_BLOCK, 4, main>, TSegSpanVecAccessor> (spans);
        Classify(spans);
        merged |= Merge<TNodeToNodeMergeDesider<3, BST_BLOCK, 2>, TSegSpanVecAccessor> (spans);
        Classify(spans);

        if (!main) {
            merged |= Merge<TNodeToNodeTypedMergeDesider<4, BST_BLOCK, 4> , TSegSpanVecAccessor> (spans);
            Classify(spans);
        }
    }

    return spans;
}

}
