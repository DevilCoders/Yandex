#include "factor_tree.h"

#include "common_factors.h"

#include <library/cpp/domtree/domtree.h>

namespace NSegmentator {

// TFactorTree
TFactorTree::TFactorTree(NDomTree::TDomTreePtr domTreePtr)
    : DomTreePtr(domTreePtr)
{
    BuildFactorTree();
}

void TFactorTree::BuildFactorTree() {
    Nodes.clear();
    Nodes.reserve(DomTreePtr->NodeCount());
    for (const NDomTree::IDomNode& node : DomTreePtr->PreorderTraversal()) {
        Nodes.push_back(TFactorNode(&node));
    }

    size_t nodeId = 0;
    for (const NDomTree::IDomNode& node : DomTreePtr->PreorderTraversal()) {
        const NDomTree::IDomNode* domNode = node.Parent();
        if (nullptr != domNode) {
            Nodes[nodeId].Parent = GetNode(domNode->Id());
        }
        domNode = node.Next();
        if (nullptr != domNode) {
            Nodes[nodeId].Next = GetNode(domNode->Id());
        }
        domNode = node.FirstChild();
        if (nullptr != domNode) {
            Nodes[nodeId].FirstChild = GetNode(domNode->Id());
        }
        ++nodeId;
    }

    CalcMetaFactors(*this);
}

TFactorNode* TFactorTree::CalcNodeFromRight(TFactorNode* factorNode) {
    while (nullptr != factorNode && nullptr == factorNode->Next) {
        factorNode = factorNode->Parent;
    }
    return (nullptr == factorNode) ? nullptr : factorNode->Next;
}

TFactorNode* TFactorTree::CalcLowestCommonAncestor(TFactorNode* first, TFactorNode* second) {
    Y_ASSERT(nullptr != first && nullptr != second);
    bool firstIsDeeper = first->MetaFactors.RootDist > second->MetaFactors.RootDist;
    if (!firstIsDeeper) {
        std::swap(first, second);
    }
    while (first->MetaFactors.RootDist > second->MetaFactors.RootDist) {
        first = first->Parent;
    }
    while (first != second) {
        first = first->Parent;
        second = second->Parent;
    }
    return first;
}

bool TFactorTree::HasNonEmptyBlockDeeper(TFactorNode* factorNode) {
    Y_ASSERT(nullptr != factorNode);
    if (factorNode->MetaFactors.NonEmptyBlockDeeper.IsActive()) {
        return factorNode->MetaFactors.NonEmptyBlockDeeper.Get();
    }
    TFactorNode* child = factorNode->FirstChild;
    while (nullptr != child) {
        if (child->Node->IsElement()) {
            if (!child->IsEmpty()) {
                const NHtml::TTag& tag = NHtml::FindTag(child->Node->Tag());
                if (!IsInlineTag(tag) || HasNonEmptyBlockDeeper(child)) {
                    return factorNode->MetaFactors.NonEmptyBlockDeeper.Set(true);
                }
            }
        }
        child = child->Next;
    }
    return factorNode->MetaFactors.NonEmptyBlockDeeper.Set(false);
}


}  // NSegmentator
