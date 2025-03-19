#include "attr_restrictions_filter.h"


namespace NAttributes {


TAttrIterator<TAttrRestrictionsFilter::THitSearcher>* TAttrRestrictionsFilter::CreateIterator(
    const NDoom::TAttributesHitRange& range,
    const TString& attr,
    THashMap<TString, size_t>* iteratorByAttr)
{
    if (AttrIterators_.size() >= MaxAttrIterators) {
        return nullptr;
    }
    AttrIterators_.push_back(MakeHolder<TAttrIterator<THitSearcher>>(HitSearcher_, range.Start(), range.End(), MaxSmallIteratorDocs_));
    iteratorByAttr->emplace(attr, AttrIterators_.size() - 1);
    return AttrIterators_.back().Get();
}


bool TAttrRestrictionsFilter::IsSegmentAttr(const NAttributes::TAttrRestrictionIteratorProto& restriction, const THashMap<TString, TRange>* segTreeAttributeRanges) {
    if (!segTreeAttributeRanges || (restriction.GetRestriction().GetRight().empty() && restriction.GetRestriction().GetCmpOper() == NAttributes::TSingleAttrRestriction::cmpEQ)) {
        return false;
    }
    for (const auto& attr : *segTreeAttributeRanges) {
        if (restriction.GetRestriction().GetLeft().StartsWith(attr.first)) {
            return true;
        }
    }
    return false;
}

TString TAttrRestrictionsFilter::ExtractSegmentAttribute(const NAttributes::TAttrRestrictionIteratorProto& restriction, const THashMap<TString, TRange>* segTreeAttributeRanges) {
    if (!segTreeAttributeRanges) {
        return "";
    }
    for (const auto& attr : *segTreeAttributeRanges) {
        if (restriction.GetRestriction().GetLeft().StartsWith(attr.first)) {
            return attr.first;
        }
    }
    return "";
}

void TAttrRestrictionsFilter::MakeOr(size_t nodeIndex, ui32 iterIndex) {
    TAttrRestrictionIterator restriction;
    restriction.Base.MutableRestriction()->SetTreeOper(NAttributes::TSingleAttrRestriction::Leaf);
    restriction.Base.SetIterIndex(iterIndex == Max<ui32>() ? AttrIterators_.size() - 1 : iterIndex);
    AttrRestrictions_.AddNode(std::move(restriction.Base));

    // for only attribute quieries with asterisks
    if (AttrRestrictions_[nodeIndex].GetRestriction().GetTreeOper() == NAttributes::TSingleAttrRestriction::And
        || AttrRestrictions_[nodeIndex].GetRestriction().GetTreeOper() == NAttributes::TSingleAttrRestriction::Leaf)
    {
        for (ui32 child : AttrRestrictions_[nodeIndex].GetRestriction().GetChildren()) {
            AttrRestrictions_[child].MutableRestriction()->SetRequired(true);
        }
    }

    AttrRestrictions_[nodeIndex].MutableRestriction()->SetTreeOper(NAttributes::TSingleAttrRestriction::Or);
    AttrRestrictions_[nodeIndex].MutableRestriction()->AddChildren(AttrRestrictions_.Size() - 1);
}

void TAttrRestrictionsFilter::AddSegmentIteratorByIndex(
    size_t index,
    size_t nodeIndex,
    THashMap<TString, size_t>* iteratorByAttr)
{
    NDoom::TAttributesHitRange range = FlatSearcher_->ReadData(index);

    const NAttributes::TAttrRestrictionIteratorProto& restriction = AttrRestrictions_[nodeIndex];

    const TString attr = restriction.GetRestriction().GetLeft() + (restriction.GetRestriction().GetRight().empty() ? "" : ".." + restriction.GetRestriction().GetRight());
    auto iter = CreateIterator(range, attr, iteratorByAttr);
    if (iter) {
        MakeOr(nodeIndex);
    }
}


bool TAttrRestrictionsFilter::AddSegmentIterators(
    size_t nodeIndex,
    const TString& attrPrefix,
    size_t begin,
    size_t end,
    THashMap<TString, size_t>* iteratorByAttr)
{
    const NAttributes::TAttrRestrictionIteratorProto& restriction = AttrRestrictions_[nodeIndex];
    // invalid range
    if (begin >= end) {
        return false;
    }

    TKeyIterator keyIter;
    TStringBuf ref;
    NDoom::TAttributesHitRange range;
    size_t indexStart = 0;

    // invalid LowerBound
    if (!KeySearcher_->LowerBound(restriction.GetRestriction().GetLeft(), &ref, &range, &keyIter, &indexStart)) {
        return false;
    }

    // if prefix and we found not fetchable doc by lower bound then we don't have docs
    if (restriction.GetRestriction().GetLeft() == attrPrefix) {
        if (!ref.StartsWith(attrPrefix)) {
            return false;
        }
    }

    // if lowerBound got higher, we either have \xff in index or we are invalid, we believe that \xff cannot be here, so invalid
    if (indexStart >= end) {
        return false;
    }

    Y_ASSERT(indexStart < end && indexStart >= begin);

    // if GREATER THAN and we Found the same doc by LowerBound, we should increase the starting position
    if (restriction.GetRestriction().GetCmpOper() == NAttributes::TSingleAttrRestriction::cmpGT && restriction.GetRestriction().GetLeft() == ref) {
        ++indexStart;
        // it can happen although
        if (indexStart == end) {
            return false;
        }
    }

    size_t indexEnd = 0;

    // bad lowerbound
    if (!KeySearcher_->LowerBound(restriction.GetRestriction().GetRight(), &ref, &range, &keyIter, &indexEnd)) {
        return false;
    }

    // it happens when we have restriction.Restriction.Right == #date\xff e.g.
    if (indexEnd >= end) {
        indexEnd = end - 1;
    }

    // if LESS THAN and right doc found by lower bound we must decrease the segment
    if (restriction.GetRestriction().GetCmpOper() == NAttributes::TSingleAttrRestriction::cmpLT && restriction.GetRestriction().GetRight() == ref) {
        // it can happen although
        if (indexEnd == 0 || indexEnd == begin) {
            return false;
        }
        --indexEnd;
    }

    // if we found doc and it is first in range and we have LESS THAN then we don't have docs
    if (indexEnd == begin && restriction.GetRestriction().GetCmpOper() == NAttributes::TSingleAttrRestriction::cmpLT && ref == restriction.GetRestriction().GetRight()) {
        return false;
    }

    // if we are at start by Right position and are not equal to ref then we don't have docs
    if (indexEnd == begin && ref != restriction.GetRestriction().GetRight()) {
        return false;
    }

    Y_ASSERT(indexEnd >= begin && indexEnd < end);

    if (indexStart > indexEnd) {
        return false;
    }

    // segment tree with BottomUp queries, we should subtract number for FlatSearcher

    // iterators of AttrRestrictions_ are invalidated so does restriction

    indexStart = (indexStart - begin) + (end - begin - 1);
    indexEnd = (indexEnd - begin) + (end - begin - 1);
    while (indexStart < indexEnd) {
        if (!(indexStart & 1)) {
            AddSegmentIteratorByIndex(indexStart, nodeIndex, iteratorByAttr);
        }
        indexStart >>= 1;
        if (indexEnd & 1) {
            AddSegmentIteratorByIndex(indexEnd, nodeIndex, iteratorByAttr);
        }
        indexEnd = (indexEnd >> 1) - 1;
    }
    if (indexStart == indexEnd) {
        AddSegmentIteratorByIndex(indexEnd, nodeIndex, iteratorByAttr);
    }

    return true;
}

void TAttrRestrictionsFilter::ProcessIterator(
    size_t nodeIndex,
    const THashMap<TString, TRange>* segTreeAttributeRanges,
    THashMap<TString, size_t>* iteratorByAttr)
{
    TKeyIterator keyIterator;
    NDoom::TAttributesHitRange range;

    // copy because of invalidating iterators
    TString attr = AttrRestrictions_[nodeIndex].GetRestriction().GetLeft();

    // segment attrs do not end with asterisk
    if (attr.EndsWith('*')) {
        size_t openAsteriskIterators = 0;
        const TStringBuf prefix = TStringBuf(attr).Chop(1);
        TStringBuf keyBuf;
        if (KeySearcher_->LowerBound(prefix, &keyBuf, &range, &keyIterator) && keyBuf.StartsWith(prefix)) {
            while (keyIterator.ReadKey(&keyBuf, &range) && openAsteriskIterators < MaxAsteriskIterators) {
                if (!keyBuf.StartsWith(prefix)) {
                    break;
                }
                const TString key(keyBuf);
                auto it = iteratorByAttr->find(key);
                size_t iter = (it != iteratorByAttr->end() ? it->second : Max<ui32>());
                if (iter == Max<ui32>()) {
                    ++openAsteriskIterators;
                    auto iterator = CreateIterator(range, key, iteratorByAttr);
                    if (!iterator) {
                        return;
                    }
                    // now it is Or operation and iterators of AttrRestrictions_ are invalidated. So i use index here.
                    MakeOr(nodeIndex);
                } else {
                    MakeOr(nodeIndex, iter);
                }
            }
        }
    }

    // for `url:yandex.ru/*` to find `yandex.ru`
    bool exact = !attr.EndsWith("/*");

    if (!exact) {
        attr.pop_back();
        attr.pop_back();
    } else if (attr.EndsWith('*')) {
        return;
    }

    if (!FlatSearcher_ || !IsSegmentAttr(AttrRestrictions_[nodeIndex], segTreeAttributeRanges)) {
        auto it = iteratorByAttr->find(attr);
        size_t iter = (it != iteratorByAttr->end() ? it->second : Max<ui32>());
        if (iter == Max<ui32>()) {
            if (!KeySearcher_->Find(attr, &range, &keyIterator)) {
                return;
            }
            auto iterator = CreateIterator(range, attr, iteratorByAttr);
            if (!iterator) {
                return;
            }
            if (exact) {
                AttrRestrictions_[nodeIndex].SetIterIndex(AttrIterators_.size() - 1);
            } else {
                MakeOr(nodeIndex);
            }
        } else {
            if (exact) {
                AttrRestrictions_[nodeIndex].SetIterIndex(iter);
            } else {
                MakeOr(nodeIndex, iter);
            }
        }
    } else {
        if (!segTreeAttributeRanges) {
            return;
        }
        const TString attrPrefix = ExtractSegmentAttribute(AttrRestrictions_[nodeIndex], segTreeAttributeRanges);
        if (attrPrefix.empty() || !segTreeAttributeRanges->contains(attrPrefix)) {
            return;
        }
        size_t start = segTreeAttributeRanges->at(attrPrefix).first;
        size_t end = segTreeAttributeRanges->at(attrPrefix).second;
        if (!AddSegmentIterators(nodeIndex, attrPrefix, start, end, iteratorByAttr)) {
            return;
        }
    }
}

void TAttrRestrictionsFilter::ProcessIterators(
    size_t nodeIndex,
    const THashMap<TString, TRange>* segTreeAttributeRanges,
    THashMap<TString, size_t>* iteratorByAttr)
{
    // Process Iterator can modify the children, we do not need to process new children again
    size_t childrenSize = AttrRestrictions_[nodeIndex].GetRestriction().GetChildren().size();
    if (AttrRestrictions_[nodeIndex].GetRestriction().GetTreeOper() == NAttributes::TSingleAttrRestriction::Leaf) {
        ProcessIterator(nodeIndex, segTreeAttributeRanges, iteratorByAttr);
    }
    for (size_t i = 0; i < childrenSize; ++i) {
        ProcessIterators(AttrRestrictions_[nodeIndex].GetRestriction().GetChildren()[i], segTreeAttributeRanges, iteratorByAttr);
    }
}

TAttrRestrictionsFilter::TAttrRestrictionsFilter(
    const THitSearcher* hitSearcher,
    const TKeySearcher* keySearcher,
    const TSegTreeFlatSearcher* flatSearcher,
    const THashMap<TString, TRange>* segTreeAttributeRanges,
    ui32 maxSmallIteratorDocs,
    const NAttributes::TAttrRestrictions<>* restrictions)
    : KeySearcher_(keySearcher)
    , HitSearcher_(hitSearcher)
    , FlatSearcher_(flatSearcher)
    , MaxSmallIteratorDocs_(maxSmallIteratorDocs)
{
    Y_ENSURE(!restrictions->Empty());
    AttrRestrictions_.Reserve(restrictions->Size());

    for (const NAttributes::TSingleAttrRestriction& restriction : restrictions->GetBase().GetTree()) {
        TAttrRestrictionIterator node;
        *(node.Base.MutableRestriction()) = restriction;
        AttrRestrictions_.AddNode(node.Base);
    }

    // hash map needs not to create multiple iterators for the same attributes (rare case but happens in web and geosearch)
    THashMap<TString, size_t> iteratorByAttr;

    ProcessIterators(/*root=*/0, segTreeAttributeRanges, &iteratorByAttr);
}

} // namespace NAttributes
