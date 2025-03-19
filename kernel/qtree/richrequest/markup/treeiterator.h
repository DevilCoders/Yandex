#pragma once
#include <kernel/qtree/richrequest/richnode.h>

template<bool IsConst>
struct TMarkupNodeBase {
    typedef std::conditional_t<IsConst, const NSearchQuery::TMarkupItem, NSearchQuery::TMarkupItem> TMarkupType;
    typedef std::conditional_t<IsConst, const TRichRequestNode, TRichRequestNode> TParentType;

    TMarkupType* Markup;
    TParentType* Parent;

    TMarkupNodeBase(TMarkupType* markup, TParentType* parent)
        : Markup(markup)
        , Parent(parent)
    {
    }

    bool operator==(const TMarkupNodeBase& rhs) const {
        return rhs.Parent == Parent
            && rhs.Markup == Markup;
    }
};

template <bool IsConst>
class TNodeMarkupIterator {
public:
    TNodeMarkupIterator()
        : Current(0)
    {
    }

    bool operator!=(const TNodeMarkupIterator& it) const {
        return !(*this == it);
    }

    bool operator==(const TNodeMarkupIterator& it) const {
        return it.Current == Current;
    }

    TNodeMarkupIterator& operator++() {    // preincrement
        ++Current;
        return *this;
    }

    TNodeMarkupIterator& operator--() {   // predecrement
        --Current;
        return *this;
    }

    typename TMarkupNodeBase<IsConst>::TMarkupType& operator*() const noexcept {
        return *Nodes[Current].Markup;
    }

    typename TMarkupNodeBase<IsConst>::TMarkupType* operator->() const noexcept {
        return Nodes[Current].Markup;
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

    typename TMarkupNodeBase<IsConst>::TParentType& GetParent() const {
        Y_ASSERT(Current < Nodes.size() && Current != (size_t)-1);
        return *Nodes[Current].Parent;
    }

    typedef std::conditional_t<IsConst, const TRichNodePtr, TRichNodePtr> TRootType;

protected:
    template <class Traversal, class TMarkupType, class TPred>
    void Reset(TRootType& node, const TPred& pred) {
        Y_ASSERT(!!node);
        Current = 0;
        Nodes.clear();
        Traversal::template GetLeaves<TMarkupType>(node, Nodes, pred);
    }

private:
    size_t Current; //index in Nodes array.
    TVector<TMarkupNodeBase<IsConst>> Nodes;
};

class TForwardMarkupTraversal {
public:
    template <class TMarkupType, class TPred>
    static void GetLeaves(const TRichNodePtr& parent, TVector<TMarkupNodeBase<true>>& nodes, const TPred& pred) {
        typedef NSearchQuery::TForwardMarkupIterator<TMarkupType, true> TParMarkup;
        NSearchQuery::TCheckMarkupIterator<TParMarkup, TPred> mi(TParMarkup(parent->Markup()), pred);
        for(; !mi.AtEnd(); ++mi)
            nodes.push_back(TMarkupNodeBase<true>(&*mi, parent.Get()));
        for (size_t i = 0; i < parent->Children.size(); ++i)
            GetLeaves<TMarkupType>(parent->Children[i], nodes, pred);
    }

    template <class TMarkupType, class TPred>
    static void GetLeaves(TRichNodePtr& parent, TVector<TMarkupNodeBase<false>>& nodes, const TPred& pred) {
        typedef NSearchQuery::TForwardMarkupIterator<TMarkupType, false> TParMarkup;
        NSearchQuery::TCheckMarkupIterator<TParMarkup, TPred> mi(TParMarkup(parent->MutableMarkup()), pred);
        for(; !mi.AtEnd(); ++mi)
            nodes.push_back(TMarkupNodeBase<false>(&*mi, parent.Get()));
        for (size_t i = 0; i < parent->Children.size(); ++i)
            GetLeaves<TMarkupType>(parent->Children.MutableNode(i), nodes, pred);
    }
};

template <class TMarkupType, class TPred, bool IsConst = false>
class TTreeMarkupCheckIterator : public TNodeMarkupIterator<IsConst> {
public:
    TTreeMarkupCheckIterator(typename TNodeMarkupIterator<IsConst>::TRootType& node, const TPred& pred) {
        TNodeMarkupIterator<IsConst>::template Reset<TForwardMarkupTraversal, TMarkupType>(node, pred);
    }

    TMarkupType& GetData() {
        return (*this)->template GetDataAs<TMarkupType>();
    }
};

template <class TMarkupType, bool IsConst = false>
class TTreeMarkupIterator : public TTreeMarkupCheckIterator<TMarkupType, NSearchQuery::TAlwaysTrue, IsConst> {
public:
    TTreeMarkupIterator(typename TNodeMarkupIterator<IsConst>::TRootType& node)
    : TTreeMarkupCheckIterator<TMarkupType, NSearchQuery::TAlwaysTrue, IsConst>(node, NSearchQuery::TAlwaysTrue())
    {
    }
};

template <class TMarkupType, class TPred>
using TTreeMarkupCheckConstIterator = TTreeMarkupCheckIterator<TMarkupType, TPred, true>;

template <class TMarkupType>
using TTreeMarkupConstIterator = TTreeMarkupIterator<TMarkupType, true>;
