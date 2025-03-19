#pragma once

#include <util/generic/yexception.h>
#include <util/generic/ylimits.h>
#include <util/system/compiler.h>


namespace NAttributes {


template <class Searcher>
class TAttrIterator {
    using TIterator = typename Searcher::TIterator;
    using TPosition = typename Searcher::TPosition;
    using THit = typename TIterator::THit;

public:
    TAttrIterator() = default;

    TAttrIterator(const Searcher* searcher, const TPosition& start, const TPosition& end, ui32 maxDocs) {
        size_t numDocs = (end.SubIndex() - start.SubIndex()) * 64 + end.Offset().Index() - start.Offset().Index();
        IsSmall_ = (numDocs <= maxDocs);
        Y_ENSURE(searcher->Seek(start, end, &Iterator_));
    }

    // returns if docId is accepted by the iterator. for IsSmall_ mode docs must be in non-decreasing calling order
    bool IsAccepted(ui32 docId) {
        Y_ASSERT(docId != Max<ui32>());
        if (IsSmall_) {
            if (Y_LIKELY(LastHit_.DocId() != Max<ui32>())) {
                if (docId <= LastHit_.DocId()) {
                    return LastHit_.DocId() == docId;
                }
            }
            while (Iterator_.ReadHits([&](THit hit) {
                if (hit.DocId() < docId) {
                    return true;
                }
                LastHit_ = hit;
                return false;
            })) {}
        } else {
            if (!Iterator_.LowerBound(THit(docId), &LastHit_)) {
                return false;
            }
        }
        return LastHit_.DocId() == docId;
    }

    // returns the first Doc with docId strictly greater than current docId
    // For IsSmall_ mode try not to mix with the IsAccepted function unless you fully understand the invariants
    ui32 Next(ui32 docId) {
        Y_ASSERT(docId != Max<ui32>());
        if (IsSmall_) {
            if (Y_LIKELY(LastHit_.DocId() != Max<ui32>())) {
                if (docId < LastHit_.DocId()) {
                    return LastHit_.DocId();
                }
            }
            while (Iterator_.ReadHits([&](THit hit) {
                if (hit.DocId() <= docId) {
                    return true;
                }
                LastHit_ = hit;
                return false;
            })) {}
            if (LastHit_.DocId() <= docId) {
                return Max<ui32>();
            }
        } else {
            if (!Iterator_.LowerBound(THit(docId + 1), &LastHit_)) {
                return Max<ui32>();
            }
        }
        return LastHit_.DocId();
    }

private:
    TIterator Iterator_;
    THit LastHit_{Max<ui32>()};
    bool IsSmall_ = false;
};


} // namespace NAttributes
