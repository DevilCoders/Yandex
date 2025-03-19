#pragma once

#include <kernel/gazetteer/richtree/gztres.h>

#include <kernel/qtree/richrequest/richnode_fwd.h>

#include <kernel/remorph/common/gztfilter.h>
#include <kernel/remorph/common/gztoccurrence.h>
#include <kernel/remorph/input/gazetteer_input.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NReMorph {

namespace NRichNode {

// Filter function prototype. Returns true if the specified gzt item should be excluded from the result
typedef bool (*TGztResultFilterFunc)(const TGztResultPosition& );

// Filter implementation, which returns true for multi-word items separated by comma
bool IsDividedByComma(const TGztResultPosition& gztItem);


class TGztResultIter {
private:
    TVector<TGztResultPosition> Results;
    TVector<TGztResultPosition>::const_iterator Iter;
    const TGztResultFilterFunc Filter;
    // TGztResults contains positions in the TConstNodesVector. We need to map these positions
    // to offsets in our TNodeInputSymbols vector. The OffsetMapping contains offsets of each node
    // in the vector of symbols.
    const TVector<size_t> OffsetMapping;
    const bool EmptyMapping; // For optimization

private:
    inline size_t GetRealOffset(size_t off) const {
        if (Y_LIKELY(EmptyMapping))
            return off;
        Y_ASSERT(off < OffsetMapping.size());
        return OffsetMapping[off];
    }

public:
    typedef const TGztResultItem* TGztItem;

public:
    TGztResultIter(const TGztResults& gztResults, const TConstNodesVector& nodes, TGztResultFilterFunc filter = nullptr,
        const TVector<size_t>& offsetMapping = TVector<size_t>());
    TGztResultIter(const TVector<TGztResultPosition>& results, TGztResultFilterFunc filter = nullptr,
        const TVector<size_t>& offsetMapping = TVector<size_t>());

    inline bool Ok() const {
        return Iter != Results.end();
    }

    inline const TGztResultItem* operator*() const {
        Y_ASSERT(Ok());
        return &(**Iter);
    }

    TGztResultIter& operator++() {
        do {
            ++Iter;
        } while (Iter != Results.end() && nullptr != Filter && Filter(*Iter));
        return *this;
    }

    inline bool BelongsTo(const NGztSupport::TGazetteerFilter& filter) const {
        Y_ASSERT(Ok());
        return filter.Has(**Iter);
    }

    inline size_t GetStartPos() const {
        Y_ASSERT(Ok());
        return GetRealOffset(Iter->GetStartIndex());
    }

    inline size_t GetEndPos() const {
        Y_ASSERT(Ok());
        // Get real position of the last item and increment it.
        // In such way we exclude trailing punctuation from the matched range.
        return GetRealOffset(Iter->GetLastIndex()) + 1;
    }

    void Reset();

    inline TString GetDebugString() {
        Y_ASSERT(Ok());
        return TString().append((*Iter)->GetType()->name())
            .append("[")
            .append(WideToUTF8((*Iter)->GetTitle()))
            .append("]: ")
            .append(WideToUTF8((*Iter)->DebugString()));
    }
};

} // NRichNode

} // NReMorph
