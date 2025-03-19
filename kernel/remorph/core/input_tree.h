#pragma once

#include "literal.h"
#include "util.h"

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/typetraits.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>
#include <util/str_stl.h>
#include <util/system/defaults.h>
#include <utility>

namespace NRemorph {

template <typename TSymbol>
class TInputTree;

template <typename TSymbol>
using TInputTreePtr = TIntrusivePtr<TInputTree<TSymbol>>;

template <class TSymbol>
class TInputTree {
public:
    using value_type = TSymbol;

    class TSymbolIterator {
    private:
        TVector<size_t>::const_iterator TrackStart;
        TVector<size_t>::const_iterator TrackEnd;
        const TInputTree* Node;

    public:
        TSymbolIterator()
            : TrackStart()
            , TrackEnd()
            , Node(NULL)
        {
        }

        TSymbolIterator(TVector<size_t>::const_iterator start, TVector<size_t>::const_iterator end, const TInputTree* node)
            : TrackStart(start)
            , TrackEnd(end)
            , Node(node)
        {
            InitNode();
        }

        Y_FORCE_INLINE bool Ok() const {
            return nullptr != Node;
        }

        Y_FORCE_INLINE const TSymbol& operator *() const {
            Y_ASSERT(Ok());
            return Node->Symbol;
        }

        Y_FORCE_INLINE const TSymbol* operator ->() const {
            Y_ASSERT(Ok());
            return &Node->Symbol;
        }

        Y_FORCE_INLINE TSymbolIterator& operator ++() {
            Y_ASSERT(Ok());
            Y_ASSERT(TrackStart != TrackEnd);
            ++TrackStart;
            InitNode();
            return *this;
        }

        Y_FORCE_INLINE TSymbolIterator operator ++(int) {
            TSymbolIterator res = *this;
            operator ++();
            return res;
        }

    private:
        Y_FORCE_INLINE void InitNode() {
            Y_ASSERT(TrackStart == TrackEnd || *TrackStart < Node->Next.size());
            Node = TrackStart != TrackEnd && *TrackStart < Node->Next.size()
                ? &(Node->Next[*TrackStart])
                : nullptr;
        }

    };

private:
    // Used in TraverseSymbols() method to create a proper THashSet
    template <typename T>
    struct TSymbolTraits {
        using TSymbolHash = THash<T>;
        using TSymbolEqualTo = TEqualTo<T>;
    };

    // Specialization for TIntrusivePtrs
    template <typename T>
    struct TSymbolTraits<TIntrusivePtr<T>> {
        struct TSymbolHash {
            Y_FORCE_INLINE size_t operator()(const TIntrusivePtr<T>& s) const {
                return (size_t)s.Get();
            }
        };
        struct TSymbolEqualTo {
            Y_FORCE_INLINE bool operator() (const TIntrusivePtr<T>& l, const TIntrusivePtr<T>& r) {
                return l.Get() == r.Get();
            }
        };
    };

private:
    static constexpr size_t MAX_INPUT_TREE_BRANCHES = 200;

private:
    TSymbol Symbol; // Symbol itself
    size_t SymbolLength; // Symbol length (relative to the original sequence)

    TVector<TInputTree> Next; // Next symbol variants

    size_t SubBranches; // Total number of sub-branches from the current node
    bool Accepted; // Has true if current symbol or any symbol from sub-branches is accepted by the applied filter

public:
    TInputTree()
        : Symbol()
        , SymbolLength(0)
        , Next()
        , SubBranches(0)
        , Accepted(true) // New node is accepted by default
    {
    }

    TInputTree(typename TTypeTraits<TSymbol>::TFuncParam symbol, size_t symbolLength)
        : Symbol(symbol)
        , SymbolLength(symbolLength)
        , Next()
        , SubBranches(0)
        , Accepted(true) // New node is accepted by default
    {
    }

    TInputTree(typename TTypeTraits<TSymbol>::TFuncParam symbol)
        : TInputTree(symbol, 1)
    {
    }


    Y_FORCE_INLINE const TSymbol& GetSymbol() const {
        return Symbol;
    }

    Y_FORCE_INLINE size_t GetLength() const {
        return SymbolLength;
    }

    Y_FORCE_INLINE size_t GetSubBranches() const {
        return SubBranches;
    }

    Y_FORCE_INLINE const TVector<TInputTree>& GetNext() const {
        return Next;
    }

    Y_FORCE_INLINE TVector<TInputTree>& GetNext() {
        return Next;
    }

    Y_FORCE_INLINE bool IsAccepted() const {
        return Accepted;
    }

    Y_FORCE_INLINE void AddNext(const TInputTree& node) {
        Next.push_back(node);
        SubBranches += Max(node.SubBranches, (size_t)1);
    }

    Y_FORCE_INLINE void Clear() {
        Next.clear();
        SubBranches = 0;
        SymbolLength = 0;
    }

    // Returns true if has at least one branch with a depth not less than specified
    bool HasDepth(size_t depth) const {
        if (depth < 2) { // optimization
            return true;
        }
        for (typename TVector<TInputTree>::const_iterator i = Next.begin(); i != Next.end(); ++i) {
            if (i->HasDepth(depth - 1)) {
                return true;
            }
        }
        return false;
    }

    // Create a new branch for a new symbol in the specified range. Removes base branch if replace == true
    Y_FORCE_INLINE bool CreateBranch(size_t start, size_t end, typename TTypeTraits<TSymbol>::TFuncParam newSymbol, bool replace) {
        return CreateBranch(start, end, newSymbol, replace, 0);
    }

    // Create new branches for new symbols in the specified range. Removes base branch if replace == true
    Y_FORCE_INLINE bool CreateBranches(size_t start, size_t end, const TVector<TSymbol>& newSymbols, bool replace) {
        Y_ASSERT(!newSymbols.empty());
        return CreateBranches(start, end, newSymbols, replace, 0);
    }

    // Traverses all unique nodes in the input
    // Stops when action returns false
    template <class TAction>
    void TraverseSymbols(TAction& action) const {
        THashSet<TSymbol, typename TSymbolTraits<TSymbol>::TSymbolHash, typename TSymbolTraits<TSymbol>::TSymbolEqualTo, std::allocator<TSymbol>> processed;
        TVector<const TInputTree*> queue;

        // The first node is dummy. Start from second level
        for (typename TVector<TInputTree>::const_iterator i = Next.begin(); i != Next.end(); ++i) {
            queue.push_back(&*i);
        }
        while (!queue.empty()) {
            const TInputTree* c = queue.back();
            queue.pop_back();
            if (processed.insert(c->Symbol).second)
                if (!action(c->Symbol))
                    break;
            for (typename TVector<TInputTree>::const_iterator i = c->Next.begin(); i != c->Next.end(); ++i)
                queue.push_back(&*i);
        }
    }

    // Depth-tracking deep-first traversing.
    template <typename TAction>
    void TraverseNodes(TAction& action) const {
        using TEnumeratedNode = std::pair<size_t, const TInputTree<TSymbol>*>;

        TVector<TEnumeratedNode> stack;
        stack.push_back(std::make_pair(0, this));

        while (!stack.empty()) {
            std::pair<size_t, const TInputTree<TSymbol>*> item = stack.back();
            stack.pop_back();
            action(*item.second, item.first);
            const TVector<TInputTree>& children = item.second->GetNext();
            for (typename TVector<TInputTree>::const_reverse_iterator child = children.rbegin(); child != children.rend(); ++child) {
                stack.push_back(std::make_pair(item.first + 1, &(*child)));
            }
        }
    }

    // Traverses all branches in the input while the action returns true
    template <class TAction>
    Y_FORCE_INLINE bool TraverseBranches(TAction& action) const {
        TVector<TSymbol> curBranch;
        TVector<size_t> curTrack;
        // The first node is dummy. Start from second level
        bool res = true;
        for (size_t i = 0; i < Next.size() && res; ++i) {
            curTrack.assign(1, i);
            res = Next[i].TraverseBranches(action, curBranch, curTrack);
        }
        return res;
    }

    // Remove branches, which don't conform to the acceptor
    // Returns true if at least one branch remains in the input
    template <class TAcceptor>
    Y_FORCE_INLINE bool Filter(const TAcceptor& accept) {
        return CheckAndRemove(accept, true);
    }

    // Create single-branch input from the plain sequence
    template <class TIterator>
    Y_FORCE_INLINE void Fill(TIterator begin, TIterator end) {
        TInputTree* current = this;
        for (; begin != end; ++begin) {
            current->Next.assign(1, TInputTree(*begin));
            current->SubBranches = 1;
            current = &(current->Next.back());
        }
        Accepted = true;
    }

    template <class TIterator, class TFactory>
    void Convert(TIterator begin, TIterator end, const TFactory& factory) {
        TInputTree* current = this;
        for (size_t i = 0; begin != end; ++begin, ++i) {
            current->Next.assign(1, TInputTree(factory(i, *begin)));
            current->SubBranches = 1;
            current = &(current->Next.back());
        }
    }

    Y_FORCE_INLINE bool ExceedsBranchLimit() {
        return SubBranches > MAX_INPUT_TREE_BRANCHES;
    }

    TSymbolIterator Iterate(const TVector<size_t>& track, size_t start = 0, size_t end = Max<size_t>()) const {
        Y_ASSERT(start < track.size());
        Y_ASSERT(end == Max<size_t>() || end <= track.size());
        const TInputTree* from = Proceed(track.begin(), track.begin() + start, TNop());
        return TSymbolIterator(track.begin() + start, end == Max<size_t>() ? track.end() : track.begin() + end, from);
    }

    template <class TAction>
    Y_FORCE_INLINE void ExtractSymbols(TAction& act, const TVector<size_t>& track, size_t start, size_t end = Max<size_t>()) const {
        Y_ASSERT(start < track.size());
        Y_ASSERT(end == Max<size_t>() || end <= track.size());
        const TInputTree* from = Proceed(track.begin(), track.begin() + start, TNop());
        Y_ASSERT(nullptr != from);
        if (nullptr != from) {
            from->Proceed(track.begin() + start, end == Max<size_t>() ? track.end() : track.begin() + end, act);
        }
    }

    Y_FORCE_INLINE const TInputTree* GetTrackNode(const TVector<size_t>& track, size_t pos) const {
        Y_ASSERT(pos < track.size());
        return Proceed(track.begin(), track.begin() + pos + 1, TNop());
    }

private:
    bool AddTail(const TInputTree& node, size_t length) {
        if (0 == length) {
            for (typename TVector<TInputTree>::const_iterator i = node.Next.begin(); i != node.Next.end(); ++i) {
                AddNext(*i);
            }
            return true;
        }

        for (typename TVector<TInputTree>::const_iterator i = node.Next.begin(); i != node.Next.end(); ++i) {
            if (i->SymbolLength <= length && AddTail(*i, length - i->SymbolLength))
                return true;
        }

        return false;
    }

    // Removes node branches, which have sequences of the specified length
    // Returns true if all sub-branches are removed and head can be removed too
    bool RemoveSubBranches(size_t length) {
        bool res = true;
        typename TVector<TInputTree>::iterator i = Next.begin();
        SubBranches = 0;
        while (i != Next.end()) {
            if (i->SymbolLength == length || (i->SymbolLength < length && i->RemoveSubBranches(length - i->SymbolLength))) {
                i = Next.erase(i);
            } else {
                res = false;
                SubBranches += Max(i->SubBranches, (size_t)1);
                ++i;
            }
        }
        return res;
    }

    bool CreateBranch(size_t start, size_t end, typename TTypeTraits<TSymbol>::TFuncParam newSymbol, bool replace, size_t pos) {
        bool res = false;
        if (pos == start) {
            TInputTree newBranch(newSymbol, end - start);
            // Find and attach tail
            if (newBranch.AddTail(*this, end - start)) {
                // Remove sub-branches, which are covered by the created symbol range
                if (replace) {
                    RemoveSubBranches(end - start);
                }
                AddNext(newBranch);
                res = true;
            }
        } else if (pos < start) {
            SubBranches = 0;
            for (typename TVector<TInputTree>::iterator i = Next.begin(); i != Next.end() && !ExceedsBranchLimit(); ++i) {
                res = i->CreateBranch(start, end, newSymbol, replace, pos + i->SymbolLength) || res;
                SubBranches += Max(i->SubBranches, (size_t)1);
            }
        }
        return res;
    }

    bool CreateBranches(size_t start, size_t end, const TVector<TSymbol>& newSymbols, bool replace, size_t pos) {
        bool res = false;
        if (pos == start) {
            Y_ASSERT(!newSymbols.empty());
            TInputTree newBranch(newSymbols.front(), end - start);
            // Find and attach tail
            if (newBranch.AddTail(*this, end - start)) {
                // Remove sub-branches, which are covered by the created symbol range
                if (replace) {
                    RemoveSubBranches(end - start);
                }
                AddNext(newBranch);

                // Clone created branch for other symbols
                for (typename TVector<TSymbol>::const_iterator iSymbol = newSymbols.begin() + 1; iSymbol != newSymbols.end() && !ExceedsBranchLimit(); ++iSymbol) {
                    newBranch.Symbol = *iSymbol;
                    AddNext(newBranch);
                }
                res = true;
            }
        } else if (pos < start) {
            SubBranches = 0;
            for (typename TVector<TInputTree>::iterator i = Next.begin(); i != Next.end() && !ExceedsBranchLimit(); ++i) {
                res = i->CreateBranches(start, end, newSymbols, replace, pos + i->SymbolLength) || res;
                SubBranches += Max(i->SubBranches, (size_t)1);
            }
        }
        return res;
    }

    template <class TAction>
    bool TraverseBranches(TAction& action, TVector<TSymbol>& curBranch, TVector<size_t>& curTrack) const {
        curBranch.push_back(Symbol);
        bool res = true;
        if (Next.empty()) {
            res = action(curBranch, curTrack);
        } else {
            for (size_t i = 0; i < Next.size() && res; ++i) {
                curTrack.push_back(i);
                res = Next[i].TraverseBranches(action, curBranch, curTrack);
                curTrack.pop_back();
            }
        }
        curBranch.pop_back();
        return res;
    }

    template <class TAction>
    Y_FORCE_INLINE const TInputTree* Proceed(TVector<size_t>::const_iterator start,
                                 TVector<size_t>::const_iterator end,
                                 const TAction& act) const {
        Y_ASSERT(start <= end);
        const TInputTree* current = this;
        for (; start != end; ++start) {
            Y_ASSERT(*start < current->Next.size());
            if (*start >= current->Next.size())
                return nullptr;
            current = &(current->Next[*start]);
            act(current->Symbol);
        }
        return current;
    }

    template <class TAcceptor>
    bool CheckAndRemove(const TAcceptor& accept, bool remove) {
        Accepted = SymbolLength != 0 && accept(Symbol);
        bool res = false;
        typename TVector<TInputTree>::iterator i = Next.begin();
        SubBranches = 0;
        while (i != Next.end()) {
            if (i->CheckAndRemove(accept, remove && !Accepted)) {
                res = true;
            } else if (remove && !Accepted) {
                i = Next.erase(i);
                continue;
            }
            SubBranches += Max(i->SubBranches, (size_t)1);
            ++i;
        }

        Accepted = Accepted || res;
        return Accepted;
    }
};

} // NRemorph
