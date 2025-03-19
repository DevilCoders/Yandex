#include "nodesequence.h"

#include "richnode.h"

#include <util/generic/utility.h>

using namespace NSearchQuery;


bool TNodeSequence::operator == (const TNodeSequence& seq) const {
    return Proxes.size() == seq.Proxes.size() &&
           std::equal(Proxes.begin(), Proxes.end(), seq.Proxes.begin()) &&
           TRichRequestNode::CompareVectors(Nodes, seq.Nodes) &&
           Markup_ == seq.Markup_;
}

size_t TNodeSequence::FindPosition(const TRichRequestNode* node) const {
    for (size_t i = 0; i < Nodes.size(); ++i)
        if (Nodes[i].Get() == node)
            return i;
    return (size_t)-1;
}


inline TRange TNodeSequence::CopyRange(const TRange& src, int shift) const {
    TRange ret = src;
    ShiftRange(ret, shift);
    return ret;
}

template <>
void TNodeSequence::CopyMarkupFrom<true>(const TMarkup& markup, int shift) {
    for (TAllMarkupConstIterator it(markup); !it.AtEnd(); ++it)
        Markup_.Add(TMarkupItem(CopyRange(it->Range, shift), it->Data->Clone()));
}

template <>
void TNodeSequence::CopyMarkupFrom<false>(const TMarkup& markup, int shift) {
    for (TAllMarkupConstIterator it(markup); !it.AtEnd(); ++it)
        Markup_.Add(TMarkupItem(CopyRange(it->Range, shift), it->Data));
}

void TNodeSequence::Clear() {
    Nodes.clear();
    Proxes.clear();
    Markup_.Clear();
}

void TNodeSequence::CopyTo(TNodeSequence& seq) const {
    if (&seq != this) {
        seq.Clear();
        seq.Nodes.reserve(Nodes.size());
        for (size_t i = 0; i < Nodes.size(); ++i)
            seq.Nodes.push_back(Nodes[i]->Copy());

        seq.Proxes.reserve(Proxes.size());
        for (size_t i = 0; i < Proxes.size(); ++i)
            seq.Proxes.push_back(Proxes[i]);

        seq.CopyMarkupFrom<true>(Markup_, 0);
    }
}

bool TNodeSequence::IsMultitokenPart(size_t pos) const {
    return ProxBefore(pos).DistanceType == DT_MULTITOKEN ||
           ProxAfter(pos).DistanceType == DT_MULTITOKEN;
}

template <bool erase>
static void FixAffectedMarkup(TMarkup& markup, size_t start, size_t oldSize, size_t newSize) {
    // Remove/extend markup intersecting range [start, stop)
    // Shift markup located from the right side of range [start, stop)
    size_t stop = start + oldSize;
    int shift = (int)newSize - (int)oldSize;

    NSearchQuery::TAllMarkupIterator it(markup);
    while (!it.AtEnd()) {
        size_t beg = it->Range.Beg;
        size_t end = it->Range.End + 1;
        if (end <= start) {
            // markup from left side, not affected
            ++it;
        } else if (beg >= stop) {
            // markup from right side, shift
            ShiftRange(it->Range, shift);
            ++it;
        } else {
            // markup intersects affected range
            if (!erase && (oldSize == newSize || (beg <= start && end >= stop))) {
                // either no shift required or markup covers whole range (non-ambiguous transformation)
                // extend/shrink (do not forget to fix edge node ids properly afterwards, where necessary)
                it->Range.End += shift;
                ++it;
            } else
                it.Erase();
        }
    }
}

void TNodeSequence::RemoveImpl(size_t start, size_t stop, size_t proxShift) {
    Y_ASSERT(start < stop && stop <= Nodes.size());
    FixAffectedMarkup<true>(Markup_, start, stop - start, 0);

    // do not drop edge fake proxes
    if (start == 0)
        proxShift = 1;
    else if (stop == Nodes.size())
        proxShift = 0;

    Nodes.erase(Nodes.begin() + start, Nodes.begin() + stop);
    if (Nodes.empty())
        Proxes.clear();     // maintain invariant
    else
        Proxes.erase(Proxes.begin() + start + proxShift, Proxes.begin() + stop + proxShift);
}

static inline void FixRemovedRange(size_t& start, size_t& stop, size_t len) {
    start = Min(start, len);
    stop = Min(stop, len);
    if (abs((int)stop - (int)start) >= (int)len)
        throw yexception() << "Cannot remove all children.";
}

void TNodeSequence::RemoveLeft(size_t start, size_t stop) {
    FixRemovedRange(start, stop, Nodes.size());
    if (start < stop)
        RemoveImpl(start, stop, 0);
}

void TNodeSequence::RemoveRight(size_t start, size_t stop) {
    FixRemovedRange(start, stop, Nodes.size());
    if (start < stop)
        RemoveImpl(start, stop, 1);
}

void TNodeSequence::RemoveWeak(size_t start, size_t stop) {
    FixRemovedRange(start, stop, Nodes.size());
    if (start < stop) {
        int lowestBeg, highestEnd;
        bool edgeCut = (start == 0 || stop == Nodes.size());
        lowestBeg = Proxes[start].Beg;
        highestEnd = Proxes[start].End;
        for (size_t i = start + 1; i < stop; ++i) {
            lowestBeg = Min(lowestBeg, Proxes[i].Beg);
            highestEnd = Max(highestEnd, Proxes[i].End);
        }
        RemoveImpl(start, stop, 0);
        // do not modify fake proxes
        if (!Proxes.empty() && !edgeCut) {
            Proxes[start].Beg = lowestBeg;
            Proxes[start].End = highestEnd;
       }
    }
}

void TNodeSequence::Remove(size_t start, size_t stop) {
    FixRemovedRange(start, stop, Nodes.size());
    if (start < stop) {
        //delete the right proximity and
        //children and related synonyms in [start, stop - 1]
        RemoveImpl(start, stop, stop < Nodes.size() ? 1 : 0);
    } else if (stop < start) {
        //delete the left proximity and
        //children and related synonyms in [stop + 1, start]
        RemoveImpl(stop + 1, start + 1, 0);
    }
}

void TNodeSequence::Insert(size_t pos, TRichNodePtr node, const TProximity& distance) {
    pos = Min<size_t>(pos, Nodes.size());
    FixAffectedMarkup<true>(Markup_, pos, 0, 1);

    Nodes.insert(Nodes.begin() + pos, node);

    if (Proxes.empty()) {
        // ignore specified @distance for edge proxes
        Proxes.push_back(TProximity());
        Proxes.push_back(TProximity());
    } else {
        // if inserting at the beginning of sequence (pos==0), set specified @distance as distance to succeeding node, not preciding
        Proxes.insert(Proxes.begin() + Max<size_t>(pos, 1), distance);
    }
}

void TNodeSequence::Insert(size_t pos, const TNodeSequence& nodes, const TProximity& distance /*distance to preceding node*/) {
    if (this->empty()) {
        nodes.CopyTo(*this);

    } else if (!nodes.empty()) {
        pos = Min<size_t>(pos, Nodes.size());
        FixAffectedMarkup<true>(Markup_, pos, 0, nodes.size());

        Nodes.insert(Nodes.begin() + pos, nodes.Nodes.begin(), nodes.Nodes.end());
        Proxes.insert(Proxes.begin() + pos, nodes.Proxes.begin(), nodes.Proxes.end() - 1);

        // if inserting at the beginning of sequence, set specified @distance as distance to succeeding node, not preciding
        ProxBefore(pos == 0 ? nodes.size() : pos) = distance;

        // append markup from @nodes with a shift
        CopyMarkupFrom<true>(nodes.Markup_, pos);
        Markup_.Sort();
    }
}

void TNodeSequence::Replace(size_t start, size_t stop, const TRichNodePtr& node) {
    if (start < Nodes.size() && stop > start) {
        FixAffectedMarkup<true>(Markup_, start, stop - start, 1);
        Nodes[start] = node;
        RemoveLeft(start + 1, stop);
    }
}

void TNodeSequence::Replace(size_t start, size_t stop, const TNodeSequence& nodes) {
    if (nodes.empty() || start >= Nodes.size() || start >= stop)
        return;

    if (start > 0) {
        TProximity prox = ProxBefore(start);
        RemoveLeft(start, stop);
        Insert(start, nodes, prox);

    } else if (stop < Nodes.size()) {
        TProximity prox = ProxBefore(stop);
        RemoveRight(0, stop);
        Insert(0, nodes, prox);

    } else {
        // full replacement (cannot use Remove())
        nodes.CopyTo(*this);
    }
}

void TNodeSequence::EquivalentReplace(size_t start, size_t stop, const TRichNodePtr& node) {
    TMarkup tmpMarkup;

    Markup_.Swap(tmpMarkup);    // backup
    Replace(start, stop, node); // replace (without markup)
    Markup_.Swap(tmpMarkup);    // restore

    // do not remove intersecting markup, just extend its right border
    FixAffectedMarkup<false>(Markup_, start, stop - start, 1);
}

void TNodeSequence::EquivalentReplace(size_t start, size_t stop, const TNodeSequence& nodes) {
    TMarkup tmpMarkup;
    Markup_.Swap(tmpMarkup);

    Replace(start, stop, nodes);

    // move original markup back (without cloning and shifting, only extend/shrink in affected range)
    FixAffectedMarkup<false>(tmpMarkup, start, stop - start, nodes.size());
    CopyMarkupFrom<false>(tmpMarkup, 0);

    Markup_.Sort();
}

void TNodeSequence::Group(size_t start, size_t stop, TRichNodePtr groupNode) {
    stop = Min(stop, Nodes.size());
    if (start >= Nodes.size() || stop <= start)
        return;

    for (size_t i = start; i < stop; ++i)
        groupNode->Children.Append(Nodes[i], ProxBefore(i));

    EquivalentReplace(start, stop, groupNode);
    // TODO: move inner markup (i.e. which is completely inside of [start,stop) range) down to groupNode
}

void TNodeSequence::Group(size_t start, size_t stop) {
    Group(start, stop, CreateEmptyRichNode());
}

size_t TNodeSequence::FlattenOneLevel(size_t index) {
    if (Nodes[index]->Children.empty())
        return 0;

    TRichNodePtr node = Nodes[index];   // increse refcount to prevent unexpected removing of this node and its children.
    size_t initSize = Nodes.size();

    EquivalentReplace(index, index + 1, node->Children);

    Y_ASSERT(Nodes.size() >= initSize);
    return Nodes.size() - initSize;
}

size_t TNodeSequence::Flatten(size_t index) {
    Nodes[index]->Children.Flatten();
    return FlattenOneLevel(index);
}

void TNodeSequence::Flatten() {
    for (int i = Nodes.ysize() - 1; i >= 0; --i)
        Flatten(i);
}


bool TNodeSequence::VerifySize() const {
    return (Nodes.empty() && Proxes.empty()) || (Proxes.size() == Nodes.size() + 1);
}

void TNodeSequence::ResetDistances(int beg, int end, WORDPOS_LEVEL lev, TDistanceType dist) {
    for (size_t i = 1; i < Nodes.size(); ++i)
        ProxBefore(i).SetDistance(beg, end, lev, dist);
}
void TNodeSequence::ResetDistances(const TProximity& distance) {
    for (size_t i = 1; i < Nodes.size(); ++i)
        ProxBefore(i) = distance;
}

void TNodeSequence::AddMarkupImpl(size_t beg, size_t end, TMarkupDataPtr data) {
    if (Nodes.empty())
        // ignore specified range in case of empty Nodes.
        Markup_.Add(TMarkupItem(TRange(0, 0), data));
    else {
        Y_ASSERT(beg < Nodes.size());
        Y_ASSERT(end < Nodes.size());
        Markup_.Add(TMarkupItem(TRange(beg, end), data));
    }
}

static inline bool TryMergeMarkup(TMarkup& markup, size_t beg, size_t end, TMarkupDataPtr data) {
    TVector<TMarkupItem>& sameTypeItems = markup.GetItems(data->MarkupType());
    for (size_t i = 0; i < sameTypeItems.size(); ++i) {
        if (sameTypeItems[i].Range.Beg == beg && sameTypeItems[i].Range.End == end) {
            if (sameTypeItems[i].Data->Merge(*data))
                return true;
        }
    }
    return false;
}

void TNodeSequence::AddMarkup(size_t beg, size_t end, TMarkupDataPtr data) {
    if (!TryMergeMarkup(Markup_, beg, end, data))
        AddMarkupImpl(beg, end, data);
}

