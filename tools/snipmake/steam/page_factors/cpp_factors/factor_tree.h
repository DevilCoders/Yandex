#pragma once

#include "factor_node.h"

#include <library/cpp/domtree/decl.h>

#include <util/generic/iterator.h>
#include <util/generic/vector.h>

namespace NSegmentator {

class TChildIterator : public TInputRangeAdaptor<TChildIterator> {
public:
    TChildIterator(const TFactorNode* factorNode)
        : CurChild(factorNode ? factorNode->FirstChild : nullptr)
    {}

    TFactorNode* Next() {
        TFactorNode* retval = CurChild;
        if (nullptr != CurChild) {
            CurChild = CurChild->Next;
        }
        return retval;
    }

private:
    TFactorNode* CurChild;
};


class TFactorTree {
public:
    using TFactorNodes = TVector<TFactorNode>;

    TFactorNodes Nodes;

public:
    TFactorTree(NDomTree::TDomTreePtr domTreePtr);

    TFactorNode* GetRoot() {
        if (Nodes.empty()) {
            return nullptr;
        }
        return &Nodes[0];
    }

    const TFactorNode* GetRoot() const {
        if (Nodes.empty()) {
            return nullptr;
        }
        return &Nodes[0];
    }

    TFactorNode* GetNode(ui32 id) {
        Y_ASSERT(id < Nodes.size());
        return &Nodes[id];
    }

    static TFactorNode* CalcNodeFromRight(TFactorNode* factorNode);
    static TFactorNode* CalcLowestCommonAncestor(TFactorNode* first, TFactorNode* second);
    static bool HasNonEmptyBlockDeeper(TFactorNode* factorNode);

    static TChildIterator ChildTraversal(const TFactorNode* factorNode) {
        return TChildIterator(factorNode);
    }

private:
    void BuildFactorTree();

private:
    NDomTree::TDomTreePtr DomTreePtr;
};


class TSubtreeIterator : public TInputRangeAdaptor<TSubtreeIterator> {
public:
    TSubtreeIterator(TFactorNode* factorNode, const TFactorTree& factorTree, bool useSubtreeRoot = true)
        : CurNode(factorTree.Nodes.begin())
        , FirstNotInSubtreeNode(TFactorTree::CalcNodeFromRight(factorNode))
        , EndNode(factorTree.Nodes.end())
        , UseSubtreeRoot(useSubtreeRoot)
    {
        if (nullptr == factorNode) {
            CurNode = EndNode;
        } else {
            while (&*CurNode != factorNode) {
                ++CurNode;
            }
        }
    }

    TFactorNode* Next() {
        if (UseSubtreeRoot) {
            UseSubtreeRoot = false;
        } else {
            ++CurNode;
        }
        if (CurNode == EndNode || &*CurNode == FirstNotInSubtreeNode) {
            CurNode = EndNode;
            return nullptr;
        }
        return const_cast<TFactorNode*>(&*CurNode);
    }

private:
    TFactorTree::TFactorNodes::const_iterator CurNode;
    TFactorNode* FirstNotInSubtreeNode;
    TFactorTree::TFactorNodes::const_iterator EndNode;
    bool UseSubtreeRoot;
};

}  // NSegmentator
