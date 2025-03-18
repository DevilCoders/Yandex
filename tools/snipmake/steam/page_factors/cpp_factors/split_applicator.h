#pragma once

#include "applicator.h"
#include "factor_tree.h"

#include <util/generic/hash.h>
#include <util/generic/iterator.h>
#include <util/generic/string.h>
#include <util/string/cast.h>

namespace NJson {
class TJsonValue;
}
namespace NMatrixnet {
    class TMnSseInfo;
}

namespace NSegmentator {

class TSplitApplicator : public TAbstractApplicator {
public:
    TSplitApplicator(TFactorTree& factorTree, const NMatrixnet::TMnSseInfo& mnSplit, double splitResultBorder)
        : TAbstractApplicator(factorTree)
        , MnSplit(mnSplit)
        , SplitResultBorder(splitResultBorder)
    {}

    void Apply(bool useModel = true) override;

protected:
    virtual void ApplyModel();

private:
    // sets init marks for factorNode-subtree
    static void InitSplitMarks(TFactorNode* factorNode);
    void CalcFactors();
    // corrects all marks
    void SetSplitMarks(const TVector<double>& splitResults);
    void CalcNodeFactors(TFactors& factors, TFactorNode* factorNode) const;
    void CalcTopNodeFactors(TFactors& factors, const TFactors& nodeFactors, TFactorNode* factorNode) const;
    void CalcNodeExtFactors(TFactors& nodeExtFactors, const TFactors& factors) const;
    bool NeedToSplit(double splitResult) const;

private:
    const NMatrixnet::TMnSseInfo& MnSplit;
    const double SplitResultBorder;
};


class TAwareSplitApplicator : public TSplitApplicator {
public:
    TAwareSplitApplicator(TFactorTree& factorTree, const NMatrixnet::TMnSseInfo& mnSplit,
                          double splitResultBorder, const NJson::TJsonValue& correctSegmentation)
        : TSplitApplicator(factorTree, mnSplit, splitResultBorder)
        , CorrectSegmentation(correctSegmentation)
    {}

protected:
    void ApplyModel() override;

private:
    void SetNodesFakeGuids();
    void SetNodesUnsplittable();

private:
    const NJson::TJsonValue& CorrectSegmentation;
};


struct TSplitGuid {
    TString Guid;
    bool IsNative;

    TSplitGuid(const TString& guid) {
        size_t guidEndPos = guid.find('_');
        if (guidEndPos == TString::npos) {
            Guid = guid;
            IsNative = true;
        } else {
            Guid = guid.substr(0, guidEndPos);
            IsNative = false;
        }
    }

    bool operator<(const TSplitGuid& rhs) const {
        ui32 curIntGuid = 0;
        ui32 rhsIntGuid = 0;
        TryFromString<ui32>(Guid, curIntGuid);
        TryFromString<ui32>(rhs.Guid, rhsIntGuid);
        return curIntGuid < rhsIntGuid;
    }

    bool operator==(const TSplitGuid& rhs) const {
        return Guid == rhs.Guid;
    }

};

struct TSplitGuidHash {
    inline size_t operator()(const TSplitGuid& splitGuid) const {
        return ComputeHash(splitGuid.Guid);
    }
};


class TSplitNodeIterator : public TInputRangeAdaptor<TSplitNodeIterator> {
public:
    TSplitNodeIterator(TFactorTree& factorTree)
        : Nodes(factorTree.Nodes)
        , Current(Nodes.begin())
    {}

    TFactorNode* Next() {
        while (Current != Nodes.end() && nullptr == Current->SplitFactors.Get()) {
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


class TUnsplittableNodeTraverser : NDomTree::IAbstractTraverser<TFactorNode*> {
public:
    TUnsplittableNodeTraverser(TFactorTree& factorTree)
        : Nodes(factorTree.Nodes)
        , Current(Nodes.begin())
    {}

    TFactorNode* Next() override {
        while (Current != Nodes.end()) {
            if (!Current->Splittable.Get() &&
                (nullptr == Current->Parent || Current->Parent->Splittable.Get()))
            {
                break;
            }
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

}  // NSegmentator
