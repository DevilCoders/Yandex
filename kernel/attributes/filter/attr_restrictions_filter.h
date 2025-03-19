#pragma once

#include "attr_iterator.h"

#include <kernel/attributes/filter/proto/attr_restriction_iterator.pb.h>
#include <kernel/attributes/filter/proto/attr_restriction_iterators.pb.h>

#include <kernel/attributes/restrictions/attr_restrictions.h>

#include <kernel/doom/offroad_attributes_wad/offroad_attributes_wad_io.h>

#include <util/generic/ylimits.h>
#include <util/generic/hash.h>

#include <utility>

namespace NAttributes {

/*
    AttrFilter

    Open iterators, expand the restrictions (asterisks and segment attributes).
    Then checks the docId on the acceptance of this restrictions.
*/

struct TAttrRestrictionIterator {
    TAttrRestrictionIterator() {
        Base.SetIterIndex(Max<ui32>());
    }

    explicit TAttrRestrictionIterator(const TAttrRestrictionIteratorProto& proto)
        : Base(proto) {
    }

    TAttrRestrictionIteratorProto Base;
};

class IAttrRestrictionsFilter {
public:

    virtual bool IsAccepted(ui32 docId) = 0;


    virtual ~IAttrRestrictionsFilter() = default;

protected:
    enum ELogicalData {
        FALSE,
        UNKNOWN,
        TRUE
    };

    virtual ELogicalData HasDocAttribute(ui32 docId, const TAttrRestrictionIteratorProto& restriction, const bool parentsWithAndNotOper ) = 0;

    ELogicalData GetConjunction(ELogicalData lhs, ELogicalData rhs) {
        if (lhs == FALSE || rhs == FALSE) {
            return FALSE;
        }
        if (lhs == UNKNOWN || rhs == UNKNOWN) {
            return UNKNOWN;
        }
        return TRUE;
    }

    ELogicalData IsAccepted(ui32 docId, const TAttrRestrictionIteratorProto& restriction, const bool parentsWithAndNotOper) {
        switch (restriction.GetRestriction().GetTreeOper()) {
            case NAttributes::TSingleAttrRestriction::Leaf: {
                if (restriction.GetIterIndex() == Max<ui32>()) {
                    return FALSE;
                }

                // only attribute queries have children sometimes
                // no matter if a child is required or not
                ELogicalData childrenResult = IsAcceptedAllChildren(docId, restriction, parentsWithAndNotOper);
                if (childrenResult == FALSE) {
                    return FALSE;
                }

                return GetConjunction(childrenResult, HasDocAttribute(docId, restriction, parentsWithAndNotOper));
            }
            case NAttributes::TSingleAttrRestriction::And: {
                // no matter if a child is required or not
                return IsAcceptedAllChildren(docId, restriction, parentsWithAndNotOper);
            }
            case NAttributes::TSingleAttrRestriction::Or: {
                ELogicalData requiredResult = IsRequiredAllAccepted(docId, restriction, parentsWithAndNotOper);
                if (requiredResult == FALSE) {
                    return FALSE;
                }
                return GetConjunction(requiredResult, IsAcceptedOneNonRequiredChild(docId, restriction, parentsWithAndNotOper));
            }
            case NAttributes::TSingleAttrRestriction::AndNot: {
                ELogicalData requiredResult = IsRequiredAllAccepted(docId, restriction, parentsWithAndNotOper);
                if (requiredResult == FALSE) {
                    return FALSE;
                }
                ELogicalData childrenResult = IsAcceptedOneNonRequiredChild(docId, restriction, parentsWithAndNotOper);

                // invert value
                if (childrenResult == TRUE) {
                    childrenResult = FALSE;
                } else if (childrenResult == FALSE) {
                    childrenResult = TRUE;
                }

                return GetConjunction(requiredResult, childrenResult);
            }
            default: {
                break;
            }
        }
        Y_ASSERT(false);
        return FALSE;
    }

    NAttributes::TAttrRestrictions<NAttributes::TAttrRestrictionIteratorProto> AttrRestrictions_;

private:
    /*
     *  Checks if all children are accepted
     */
    ELogicalData IsAcceptedAllChildren(ui32 docId, const TAttrRestrictionIteratorProto& restriction, const bool parentsWithAndNotOper) {
        bool unknownFlag = false;
        for (ui32 child : restriction.GetRestriction().GetChildren()) {
            ELogicalData childResult = IsAccepted(docId, AttrRestrictions_[child], parentsWithAndNotOper || (restriction.GetRestriction().GetTreeOper() == NAttributes::TSingleAttrRestriction::AndNot));
            if (childResult == FALSE) {
                return FALSE;
            }
            unknownFlag |= (childResult == UNKNOWN);
        }
        return unknownFlag ? UNKNOWN : TRUE;
    }

    ELogicalData IsAcceptedOneNonRequiredChild(ui32 docId, const TAttrRestrictionIteratorProto& restriction, const bool parentsWithAndNotOper) {
        bool unknownFlag = false;
        for (ui32 child : restriction.GetRestriction().GetChildren()) {
            if (!AttrRestrictions_[child].GetRestriction().GetRequired()) {
            ELogicalData childResult = IsAccepted(docId, AttrRestrictions_[child], parentsWithAndNotOper || (restriction.GetRestriction().GetTreeOper() == NAttributes::TSingleAttrRestriction::AndNot));
                if (childResult == TRUE) {
                    return TRUE;
                }
                unknownFlag |= (childResult == UNKNOWN);
            }
        }
        return unknownFlag ? UNKNOWN : FALSE;
    }

    /*
     * Checks if all required nodes are accepted
     */
    ELogicalData IsRequiredAllAccepted(ui32 docId, const TAttrRestrictionIteratorProto& restriction, const bool parentsWithAndNotOper) {
        bool unknownFlag = false;
        for (ui32 child : restriction.GetRestriction().GetChildren()) {
            if (AttrRestrictions_[child].GetRestriction().GetRequired()) {
                ELogicalData childResult = IsAccepted(docId, AttrRestrictions_[child], parentsWithAndNotOper || (restriction.GetRestriction().GetTreeOper() == NAttributes::TSingleAttrRestriction::AndNot));
                if (childResult == FALSE) {
                    return FALSE;
                }
                unknownFlag |= (childResult == UNKNOWN);
            }
        }
        return unknownFlag ? UNKNOWN : TRUE;
    }

};

class TAttrRestrictionsFilter : public IAttrRestrictionsFilter {
    using TIo = NDoom::TOffroadAttributesIo;
    static constexpr size_t MaxAsteriskIterators = 1000;
    static constexpr size_t MaxAttrIterators = 8000;
public:
    using THitSearcher = TIo::THitSearcher;
    using THitIterator = THitSearcher::TIterator;
    using TKeySearcher = TIo::TKeySearcher;
    using TKeyIterator = TKeySearcher::TIterator;
    using TSegTreeFlatSearcher = NDoom::TAttributesSegTreeOffsetsSearcher;
    using THit = THitIterator::THit;
    using TRange = std::pair<size_t, size_t>;

    TAttrRestrictionsFilter(const THitSearcher* hitSearcher, const TKeySearcher* keySearcher, const TSegTreeFlatSearcher* flatSearcher, const THashMap<TString, TRange>* segTreeAttributeRanges, ui32 maxSmallIteratorDocs, const NAttributes::TAttrRestrictions<>* restrictions);

    /*
     * CAUTION! docs must be represented in increasing order
     */
    bool IsAccepted(ui32 docId) override {
        Y_ASSERT(!AttrRestrictions_.Empty());
        if (docId >= LowerBound_) {
            bool ok = (IAttrRestrictionsFilter::IsAccepted(docId, AttrRestrictions_[0], false) != FALSE);
            LowerBound_ = Next(docId, AttrRestrictions_[0]);
            return ok;
        }
        return false;
    }

    /*
     * Do not mix with IsAccepted function unless you fully understand the invariants of TAttrIterator
     */
    ui32 Next(ui32 docId) {
        Y_ASSERT(!AttrRestrictions_.Empty());
        return Next(docId, AttrRestrictions_[0]);
    }

protected:
    ELogicalData HasDocAttribute(ui32 docId, const TAttrRestrictionIteratorProto& restriction, const bool /* parentsWithAndNotOper */) override {
        if (AttrIterators_[restriction.GetIterIndex()]->IsAccepted(docId)) {
            return TRUE;
        }
        return FALSE;
    };

private:
    // Next returns the candidate for the next accepted docId, not accepted doc, you should check for yourself
    ui32 Next(ui32 docId, const TAttrRestrictionIteratorProto& restriction) {
        switch (restriction.GetRestriction().GetTreeOper()) {
            case NAttributes::TSingleAttrRestriction::Leaf: {
                if (restriction.GetIterIndex() == Max<ui32>()) {
                    return Max<ui32>();
                }
                ui32 res = docId;
                for (ui32 child : restriction.GetRestriction().GetChildren()) {
                    res = Max(Next(docId, AttrRestrictions_[child]), res);
                }
                return Max(AttrIterators_[restriction.GetIterIndex()]->Next(docId), res);
            }
            case NAttributes::TSingleAttrRestriction::And: {
                if (restriction.GetRestriction().GetChildren().empty()) {
                    return Max<ui32>();
                }
                ui32 res = docId;
                for (ui32 child : restriction.GetRestriction().GetChildren()) {
                    res = Max(Next(docId, AttrRestrictions_[child]), res);
                }
                return res;
            }
            case NAttributes::TSingleAttrRestriction::Or: {
                ui32 res = Max<ui32>();
                for (ui32 child : restriction.GetRestriction().GetChildren()) {
                    if (!AttrRestrictions_[child].GetRestriction().GetRequired()) {
                        res = Min(Next(docId, AttrRestrictions_[child]), res);
                    }
                }
                for (ui32 child : restriction.GetRestriction().GetChildren()) {
                    if (AttrRestrictions_[child].GetRestriction().GetRequired()) {
                        res = Max(Next(docId, AttrRestrictions_[child]), res);
                    }
                }
                return res;
            }
            // ugly hack by danlark@
            case NAttributes::TSingleAttrRestriction::AndNot: {
                return docId + 1;
            }
            default: {
                break;
            }
        }
        return Max<ui32>();
    }

    /*
     * Checks if iterator is segment
     */
    bool IsSegmentAttr(const TAttrRestrictionIteratorProto& restriction, const THashMap<TString, TRange>* segTreeAttributeRanges);

    /*
     * Helper function for extracting the value of segment restriction
     */
    TString ExtractSegmentAttribute(const TAttrRestrictionIteratorProto& restriction, const THashMap<TString, TRange>* segTreeAttributeRanges);

    /*
     * Expanding segment iterators and asterisks
     */
    void MakeOr(size_t nodeIndex, ui32 iterIndex = Max<ui32>());

    /*
     * Creating iterator itself
     */
    TAttrIterator<THitSearcher>* CreateIterator(
        const NDoom::TAttributesHitRange& range,
        const TString& attr,
        THashMap<TString, size_t>* iteratorByAttr);

    /*
     * Final process of segment iterator
     */
    void AddSegmentIteratorByIndex(
        size_t index,
        size_t nodeIndex,
        THashMap<TString, size_t>* iteratorByAttr);

    /*
     * Do the expansion with segment iterators
     */
    bool AddSegmentIterators(
        size_t nodeIndex,
        const TString& attrPrefix,
        size_t begin,
        size_t end,
        THashMap<TString, size_t>* iteratorByAttr);

    /*
     * Create one iterator, expand the asterisks
     */
    void ProcessIterator(
        size_t nodeIndex,
        const THashMap<TString, TRange>* segTreeAttributeRanges,
        THashMap<TString, size_t>* iteratorByAttr);

    /*
     * Create all iterators for the tree
     */
    void ProcessIterators(
        size_t nodeIndex,
        const THashMap<TString, TRange>* segTreeAttributeRanges,
        THashMap<TString, size_t>* iteratorByAttr);

    TVector<THolder<TAttrIterator<THitSearcher>>> AttrIterators_;
    const TKeySearcher* KeySearcher_ = nullptr;
    const THitSearcher* HitSearcher_ = nullptr;
    const TSegTreeFlatSearcher* FlatSearcher_ = nullptr;

    // need for small iterators, in small iterators we do linear search because it is much faster for panther search
    const ui32 MaxSmallIteratorDocs_ = 0;
    ui32 LowerBound_ = 0;
};


} // namespace NAttributes
