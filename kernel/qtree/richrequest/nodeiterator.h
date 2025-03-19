#pragma once

#include <library/cpp/tokenclassifiers/token_types.h>

#include "richnode.h"


struct TLeafNode {
    using TPath = TVector<size_t>;
    /** @brief Indexes of children to get from the tree root to this leaf
     * does not account misc_ops and synonyms as children,
     * so for the leaves that are children of miscOps or synonyms,
     * you'll get confusing results.
     */
    TPath Path;
    TRichRequestNode* Parent;
    TRichRequestNode* Leaf;

    TLeafNode(TPath nodePath, TRichRequestNode* parent, TRichRequestNode* leaf)
        : Path(std::move(nodePath))
        , Parent(parent)
        , Leaf(leaf)
    {
    }
};

typedef TVector<TLeafNode> TLeafNodes;

template <bool IsConst>
class TBaseRichNodeIterator {
    typedef std::conditional_t<IsConst, const TRichRequestNode, TRichRequestNode> TValue;
public:

    bool operator!=(const TBaseRichNodeIterator& it) const {
        return !(*this == it);
    }

    bool operator==(const TBaseRichNodeIterator& it) const {
        return it.Current == Current;
    }

    TBaseRichNodeIterator& operator++() {    // preincrement
        ++Current;
        return *this;
    }

    TBaseRichNodeIterator& operator--() {   // predecrement
        --Current;
        return *this;
    }

    TValue& operator*() const noexcept {
        return *Nodes[Current].Leaf;
    }

    TValue* operator->() const noexcept {
        return Nodes[Current].Leaf;
    }

    size_t NodeCount() const noexcept {
        return Nodes.size();
    }

    bool Empty() const {
        return Nodes.empty();
    }

    bool IsDone() const {
        return Current >= Nodes.size();
    }

    bool IsFirst() const {
        return Current == 0 && !Nodes.empty();
    }

    bool IsLast() const {
        return Current + 1 == Nodes.size();
    }

    TValue& GetParent() const {
        return *Nodes[Current].Parent;
    }

    const TLeafNode::TPath& GetPath() const {
        return Nodes[Current].Path;
    }

    /// Generate a human-readable dump of all the nodes (1 per line, plain text)
    void PrintAll(IOutputStream& str) const {
        for (const TLeafNode& node : Nodes) {
            str << "/" << JoinStrings(node.Path.begin(), node.Path.end(), "/")
                       << ": " << node.Leaf->Print() << "\n";
        }
    }

    // Collect all nodes in order of iteration
    TVector<const TValue*> Collect() const {
        TVector<const TValue*> ret;
        ret.reserve(Nodes.size());
        for (size_t i = 0; i < Nodes.size(); ++i)
            ret.push_back(Nodes[i].Leaf);
        return ret;
    }

    // Collect all mutable nodes in order of iteration
    TVector<TValue*> CollectMutable() const {
        TVector<TValue*> ret;
        ret.reserve(Nodes.size());
        for (size_t i = 0; i < Nodes.size(); ++i)
            ret.push_back(Nodes[i].Leaf);
        return ret;
    }

    const TVector<TLeafNode>& CollectLeaves() const {
        return Nodes;
    }

protected:
    template <class Traversal, class TNodePredicate>
    void InitLeaves(const TRichRequestNode* node, TNodePredicate pred) {
        // we are not going to modify @node, for simplicity
        return InitLeaves<Traversal, TNodePredicate>(const_cast<TRichRequestNode*>(node), pred);
    }

    template <class Traversal, class TNodePredicate>
    void InitLeaves(TRichRequestNode* node, TNodePredicate pred) {
        TLeafNode::TPath path;
        path.reserve(4);
        Traversal::GetLeaves(node, path, node, Nodes, pred);
    }

private:
    size_t Current = 0; //index in Nodes array.
    TVector<TLeafNode> Nodes;
};


typedef bool (*TNodePredicate)(const TRequestNodeBase& n);

template <class Traversal, TNodePredicate pred, bool IsConst = false>
class TTIterator : public TBaseRichNodeIterator<IsConst> {
    typedef TBaseRichNodeIterator<IsConst> TBase;
public:
    TTIterator(TRichNodePtr node) {
        TBase::template InitLeaves<Traversal, TNodePredicate>(node.Get(), pred);
    }

    TTIterator(std::conditional_t<IsConst, const TRichRequestNode, TRichRequestNode>& node) {
        TBase::template InitLeaves<Traversal, TNodePredicate>(&node, pred);
    }
};

typedef bool (*TRichNodePredicate)(const TRichRequestNode& n);

template <class Traversal, TRichNodePredicate pred, bool IsConst = false>
class TTRichNodeIterator: public TBaseRichNodeIterator<IsConst> {
    typedef TBaseRichNodeIterator<IsConst> TBase;
public:
    TTRichNodeIterator(TRichNodePtr& node) {
        TBase::template InitLeaves<Traversal, TRichNodePredicate>(node.Get(), pred);
    }

    TTRichNodeIterator(std::conditional_t<IsConst, const TRichRequestNode, TRichRequestNode>& node) {
        TBase::template InitLeaves<Traversal, TRichNodePredicate>(&node, pred);
    }
};

class TForwardChildrenAndSynonymsTraversal {
public:
    template <class TPredicate>
    static void GetLeaves(TRichRequestNode* node, TLeafNode::TPath& path, TRichRequestNode* parent, TLeafNodes& nodes, TPredicate pred) {
        if (pred(*node)) {
            nodes.push_back(TLeafNode(path, parent, node));
            if (node->Children.empty()) {
                for (NSearchQuery::TForwardMarkupIterator<TSynonym, false> it(node->MutableMarkup()); !it.AtEnd(); ++it) {
                    if (RangeContains(it->Range, 0)) {
                        GetLeaves(it.GetData().SubTree.Get(), path, node, nodes, pred);
                    }
                }
            }
        }
        for (size_t i = 0; i < node->Children.size(); ++i) {
            path.push_back(i);
            GetLeaves(node->Children[i].Get(), path, node, nodes, pred);
            path.pop_back();
            for (NSearchQuery::TForwardMarkupIterator<TSynonym, false> it(node->MutableMarkup()); !it.AtEnd(); ++it) {
                if (RangeContains(it->Range, i))
                    GetLeaves(it.GetData().SubTree.Get(), path, node, nodes, pred);
            }
        }
    }
};

class TForwardChildrenTraversal {
public:
    template <class TPredicate>
    static void GetLeaves(TRichRequestNode* node, TLeafNode::TPath& path, TRichRequestNode* parent, TLeafNodes& nodes, TPredicate pred) {
        if (pred(*node))
            nodes.push_back(TLeafNode(path, parent, node));
        for (size_t i = 0; i < node->Children.size(); ++i) {
            path.push_back(i);
            GetLeaves(node->Children[i].Get(), path, node, nodes, pred);
            path.pop_back();
        }
    }
};

template <bool useMiskOps, bool skipCovered, EThesExtType synType>
class TForwardChildrenAndTypedSynTraversal {
public:
    template <class TPredicate>
    static void GetLeaves(TRichRequestNode* node, TLeafNode::TPath& path, TRichRequestNode* parent, TLeafNodes& nodes, TPredicate pred) {
        if (pred(*node)) {
            nodes.push_back(TLeafNode(path, parent, node));
        }
        for (size_t i = 0; i < node->Children.size();) {
            path.push_back(i);
            size_t covered = CallForSyn(node, path, i, nodes, pred);
            if (skipCovered && covered) {
                i += covered;
            } else {
                GetLeaves(node->Children[i].Get(), path, node, nodes, pred);
                ++i;
            }
            path.pop_back();
        }
        if (useMiskOps) {
            for (size_t i = 0; i < node->MiscOps.size(); ++i)
                GetLeaves(node->MiscOps[i].Get(), path, node, nodes, pred);
        }
    }
private:
    template <class TPredicate>
    static size_t CallForSyn(TRichRequestNode* node, TLeafNode::TPath& path, size_t childNum, TLeafNodes& nodes, TPredicate pred) {
        size_t covered = 0;
        typedef NSearchQuery::TForwardMarkupIterator<TSynonym, false> TSynonymIterator;
        NSearchQuery::TCheckMarkupIterator<TSynonymIterator, NSearchQuery::TSynonymTypeCheck> it (
            TSynonymIterator(node->MutableMarkup()),
            NSearchQuery::TSynonymTypeCheck(synType)
            );
        for (; !it.AtEnd(); ++it) {
            if (RangeContains(it->Range, childNum)) {
                covered = it->Range.Size();
                GetLeaves(it.GetData().SubTree.Get(), path, node, nodes, pred);
            }
        }
        return covered;
    }
};

class TReverseAllNodesTraversal {
public:
    template <class TPredicate>
    static void GetLeaves(TRichRequestNode* node, TLeafNode::TPath& path, TRichRequestNode* parent, TLeafNodes& nodes, TPredicate pred) {
        for (size_t i = 0; i < node->MiscOps.size(); ++i)
            GetLeaves(node->MiscOps[i].Get(), path, node, nodes, pred);
        for (NSearchQuery::TForwardMarkupIterator<TSynonym, false> it(node->MutableMarkup()); !it.AtEnd(); ++it)
            GetLeaves(it.GetData().SubTree.Get(), path, node, nodes, pred);
        for (NSearchQuery::TForwardMarkupIterator<TTechnicalSynonym, false> it(node->MutableMarkup()); !it.AtEnd(); ++it)
            GetLeaves(it.GetData().SubTree.Get(), path, node, nodes, pred);
        for (size_t i = 0; i < node->Children.size(); ++i) {
            path.push_back(i);
            GetLeaves(node->Children[i].Get(), path, node, nodes, pred);
            path.pop_back();
        }
        if (pred(*node))
            nodes.push_back(TLeafNode(path, parent, node));
    }
};

class TReverseChildrenAndMiscOpsTraversal {
public:
    template <class TPredicate>
    static void GetLeaves(TRichRequestNode* node, TLeafNode::TPath& path, TRichRequestNode* parent, TLeafNodes& nodes, TPredicate pred) {
        for (size_t i = 0; i < node->MiscOps.size(); ++i)
            GetLeaves(node->MiscOps[i].Get(), path, node, nodes, pred);
        for (size_t i = 0; i < node->Children.size(); ++i) {
            path.push_back(i);
            GetLeaves(node->Children[i].Get(), path, node, nodes, pred);
            path.pop_back();
        }
        if (pred(*node))
            nodes.push_back(TLeafNode(path, parent, node));
    }
};

class TReverseChildrenTraversal {
public:
    template <class TPredicate>
    static void GetLeaves(TRichRequestNode* node, TLeafNode::TPath& path, TRichRequestNode* parent, TLeafNodes& nodes, TPredicate pred) {
        for (int i = (int)node->Children.size() - 1; i >= 0; --i) {
            path.push_back(i);
            GetLeaves(node->Children[i].Get(), path, node, nodes, pred);
            path.pop_back();
        }
        for (NSearchQuery::TBackwardMarkupIterator<TSynonym, false> it(node->MutableMarkup()); !it.AtEnd(); ++it)
            GetLeaves(it.GetData().SubTree.Get(), path, node, nodes, pred);
        if (pred(*node))
            nodes.push_back(TLeafNode(path, parent, node));
    }
};

template <bool useMarkup = true>
class TForwardPredicateFilteringChildrenTraversal {
public:
    template <class TPredicate>
    static void GetLeaves(TRichRequestNode* node, TLeafNode::TPath& path, TRichRequestNode* parent, TLeafNodes& nodes, TPredicate pred) {
        if (pred(*node))
            nodes.push_back(TLeafNode(path, parent, node));
        else {
            for (size_t i = 0; i < node->Children.size(); ++i) {
                path.push_back(i);
                GetLeaves(node->Children[i].Get(), path, node, nodes, pred);
                path.pop_back();
            }
            if (useMarkup)
                for (NSearchQuery::TForwardMarkupIterator<TSynonym, false> it(node->MutableMarkup()); !it.AtEnd(); ++it)
                    GetLeaves(it.GetData().SubTree.Get(), path, node, nodes, pred);
        }
    }
};


typedef TTIterator<TForwardChildrenTraversal, AlwaysTrue> TRichNodeIterator;
typedef TTIterator<TForwardChildrenTraversal, IsUserPhrase> TUserPhraseIterator;
typedef TTIterator<TForwardChildrenAndSynonymsTraversal, IsWordAndNotEmoji> TWordIterator;
typedef TTIterator<TReverseAllNodesTraversal, AlwaysTrue> TAllNodesIterator;
typedef TTIterator<TReverseAllNodesTraversal, IsAttribute> TAttributeIterator;
typedef TTIterator<TReverseAllNodesTraversal, IsAttribute, true> TAttributeConstIterator;
typedef TTIterator<TReverseAllNodesTraversal, IsWordAndNotEmoji> TAllWordIterator;
typedef TTIterator<TReverseAllNodesTraversal, IsWordAndNotEmoji, true> TAllWordConstIterator;
typedef TTIterator<TForwardPredicateFilteringChildrenTraversal<true>, IsQuote> TQuoteIterator;
typedef TTIterator<TForwardChildrenTraversal, IsAndOp> TAndIterator;
typedef TTIterator<TForwardPredicateFilteringChildrenTraversal<true>, IsWordOrMultitoken> TMultiWordIterator;
typedef TTIterator<TForwardChildrenTraversal, IsWordAndNotEmoji> TUserWordsIterator;
typedef TTIterator<TForwardChildrenTraversal, IsWordAndNotEmoji, true> TUserWordsConstIterator;
typedef TTIterator<TReverseAllNodesTraversal, IsOr> TOrIterator;
typedef TTIterator<TForwardChildrenTraversal, IsClassifiedToken<NTokenClassification::ETT_EMAIL> > TEmailIterator;
typedef TTIterator<TForwardChildrenTraversal, IsClassifiedToken<NTokenClassification::ETT_URL> >   TUrlIterator;
typedef TTIterator<TForwardChildrenTraversal, IsClassifiedToken> TClassifiedTokenIterator;
typedef TTIterator<TForwardChildrenTraversal, IsClassifiedToken, true> TClassifiedTokenConstIterator;

// Like TUserWordsIterator, but does not split merged marks (like "N99")
typedef TTIterator<TForwardChildrenAndTypedSynTraversal<false, true, TE_MARK>, IsWord> TMergedWordIterator;
typedef TTIterator<TForwardChildrenAndTypedSynTraversal<false, true, TE_MARK>, IsWord, true> TMergedWordConstIterator;

// Больше упячки, хорошей и разной!
template <class TPredicate>
void GetChildNodes(const TRichRequestNode& node, TConstNodesVector& nodes, TPredicate pred, bool treatSub = true) {
    if (pred(node)) {
        nodes.push_back(&node);
        if (!treatSub)
            return;
    }
    TVector<TRichNodePtr>::const_iterator it, end;
    end = node.Children.end();
    for (it = node.Children.begin(); it != end; ++it)
        GetChildNodes(**it, nodes, pred, treatSub);
}
