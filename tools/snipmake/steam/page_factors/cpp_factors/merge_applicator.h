#pragma once

#include "applicator.h"
#include "split_applicator.h"
#include "page_segment.h"

#include <util/generic/iterator.h>

namespace NMatrixnet {
    class TMnSseInfo;
}

namespace NSegmentator {

class TMergeApplicator : public TAbstractApplicator {
public:
    TMergeApplicator(TFactorTree& factorTree, const NMatrixnet::TMnSseInfo& mnMerge, double mergeResultBorder)
        : TAbstractApplicator(factorTree)
        , MnMerge(mnMerge)
        , MergeResultBorder(mergeResultBorder)
    {}

    void Apply(bool useModel = true) override;

protected:
    virtual void ApplyModel();
    static void CorrectFactors(TFactors* segmentFactors, ui32 prevMergeCount);

private:
    // sets factors into the 2nd node when asking about merge for 1st and 2nd nodes
    void CalcFactors();
    // sets marks for end nodes for merge
    void SetMergeMarks(const TVector<bool>& mergeResults);
    void CalcNodeFactors(TFactors& factors, TFactorNode* factorNode) const;
    void CalcTopNodeFactors(TFactors& factors, const TFactors& curNodeFactors, const TFactors& prevNodeFactors) const;
    void CalcNodeExtFactors(TFactors& nodeExtFactors, const TFactors& factors) const;
    bool NeedToMerge(double mergeResult) const;

private:
    const NMatrixnet::TMnSseInfo& MnMerge;
    const double MergeResultBorder;
};


class TAwareMergeApplicator : public TMergeApplicator {
public:
    TAwareMergeApplicator(TFactorTree& factorTree, const NMatrixnet::TMnSseInfo& mnMerge,
                          double mergeResultBorder, const NJson::TJsonValue& correctSegmentation)
        : TMergeApplicator(factorTree, mnMerge, mergeResultBorder)
        , CorrectSegmentation(correctSegmentation)
    {}

protected:
    void ApplyModel() override;

private:
    const NJson::TJsonValue& CorrectSegmentation;
};


class TMergeNodeIterator : public TInputRangeAdaptor<TMergeNodeIterator> {
public:
    TMergeNodeIterator(TFactorTree& factorTree)
        : Nodes(factorTree.Nodes)
        , Current(Nodes.begin())
    {}

    TFactorNode* Next() {
        while (Current != Nodes.end() && nullptr == Current->MergeFactors.Get()) {
            ++Current;
        }
        TFactorNode* res = (Current == Nodes.end() ? nullptr : &*Current);
        ++Current;
        return res;
    }

private:
    TFactorTree::TFactorNodes& Nodes;
    TFactorTree::TFactorNodes::iterator Current;
};


class TMergeSegmentTraverser : NDomTree::IAbstractTraverser<TPageSegment> {
public:
    TMergeSegmentTraverser(TFactorTree& factorTree)
        : UnsplittableNodes(factorTree)
        , CurrentFirstNode(UnsplittableNodes.Next())
    {}

    TPageSegment Next() override {
        if (nullptr == CurrentFirstNode) {
            return TPageSegment(nullptr);
        }
        TFactorNode* prevNode = CurrentFirstNode;
        TFactorNode* currentNode = UnsplittableNodes.Next();
        while (nullptr != currentNode && currentNode->MergeId == CurrentFirstNode->MergeId) {
            prevNode = currentNode;
            currentNode = UnsplittableNodes.Next();
        }
        TPageSegment res(CurrentFirstNode, prevNode);
        CurrentFirstNode = currentNode;
        return res;
    }

private:
    TUnsplittableNodeTraverser UnsplittableNodes;
    TFactorNode* CurrentFirstNode;
};

}  // NSegmentator
