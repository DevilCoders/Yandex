#pragma once

#include "applicator.h"
#include "merge_applicator.h"

#include <util/generic/iterator.h>
#include <util/generic/map.h>
#include <util/generic/array_ref.h>

namespace NMatrixnet {
    class TMnMultiCateg;
}

namespace NSegmentator {

class TAnnotateApplicator : public TAbstractApplicator {
public:
    TAnnotateApplicator(TFactorTree& factorTree, const NMatrixnet::TMnMultiCateg& mnAnnotate)
        : TAbstractApplicator(factorTree)
        , MnAnnotate(mnAnnotate)
    {}

    void Apply(bool useModel = true) override;

protected:
    virtual void ApplyModel();

private:
    void CalcFactors();
    // sets marks for the first node of each merged page segment
    void SetTypeMarks(const double* annotateResults);
    void CalcNodeFactors(TFactors& factors, TFactorNode* factorNode) const;
    void CalcTopNodeFactors(TFactors& factors, const TFactors& nodeFactors) const;
    void CalcNodeExtFactors(TFactors& nodeExtFactors, const TFactors& factors) const;
    static ui32 CalcSegmentTypeId(const double* result, TConstArrayRef<double> categValues);

private:
    const NMatrixnet::TMnMultiCateg& MnAnnotate;
};


class TAwareAnnotateApplicator : public TAnnotateApplicator {
public:
    TAwareAnnotateApplicator(TFactorTree& factorTree, const NMatrixnet::TMnMultiCateg& mnAnnotate,
                             const NJson::TJsonValue& correctSegmentation,
                             const TMap<TString, ui32>& classes2Codes)
        : TAnnotateApplicator(factorTree, mnAnnotate)
        , CorrectSegmentation(correctSegmentation)
        , Classes2Codes(classes2Codes)
    {}

protected:
    void ApplyModel() override;

private:
    const NJson::TJsonValue& CorrectSegmentation;
    const TMap<TString, ui32>& Classes2Codes;
};


class TAnnotateNodeIterator : public TInputRangeAdaptor<TAnnotateNodeIterator> {
public:
    TAnnotateNodeIterator(TFactorTree& factorTree)
        : Nodes(factorTree.Nodes)
        , Current(Nodes.begin())
    {}

    TFactorNode* Next() {
        while (!(Current == Nodes.end() || nullptr != Current->AnnotateFactors.Get())) {
            ++Current;
        }
        TFactorNode* res =  (Current == Nodes.end() ? nullptr : &*Current);
        ++Current;
        return res;
    }

private:
    TFactorTree::TFactorNodes& Nodes;
    TFactorTree::TFactorNodes::iterator Current;
};


class TAnnotateSegmentTraverser : NDomTree::IAbstractTraverser<TPageSegment> {
public:
    TAnnotateSegmentTraverser(TFactorTree& factorTree)
        : MergeSegmentTraverser(factorTree)
    {}

    TPageSegment Next() override {
        return MergeSegmentTraverser.Next();
    }

private:
    TMergeSegmentTraverser MergeSegmentTraverser;
};

}  // NSegmentator
