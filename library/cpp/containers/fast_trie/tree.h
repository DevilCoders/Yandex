#pragma once

#include <util/memory/pool.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/typetraits.h>

#include "indexed_array.h"

// Base traits for tree with array-contained node children
template <typename ArrayTraitsType>
struct TBaseArrayTreeTraits: public ArrayTraitsType {
    typedef const typename ArrayTraitsType::TKey* TKeyIter;

    template <typename TNode>
    class TChildren: public TIndexedArray<TNode*, ArrayTraitsType> {
        typedef TIndexedArray<TNode*, ArrayTraitsType> TBase;

    public:
        TChildren(TMemoryPool*) {
        }
    };
};
using TCharArrayTreeTraits = TBaseArrayTreeTraits<TCharArrayTraits>;

// Base traits for tree with hash-contained node children
template <typename KeyType, typename HashType = THash<KeyType>, typename EqualType = TEqualTo<KeyType>>
struct TBaseHashTreeTraits {
    typedef KeyType TKey;
    typedef const KeyType* TKeyIter;

    template <typename TNode>
    class TChildren: public THashMap<TKey, TNode*, HashType, EqualType, TPoolAllocator> {
        typedef THashMap<TKey, TNode*, HashType, EqualType, TPoolAllocator> TBase;

    public:
        TChildren(TMemoryPool* pool)
            : TBase(pool)
        {
        }
    };
};

// Two ways to iterate a range
namespace NDirections {
    struct TForward {
        template <typename T>
        static void Inc(T& begin, T& end) {
            if (begin != end)
                ++begin;
        }
        template <typename T>
        static T Get(T begin, T end) {
            Y_ASSERT(begin != end);
            return begin;
        }
    };

    struct TBackward {
        template <typename T>
        static void Inc(T& begin, T& end) {
            if (begin != end)
                --end;
        }
        template <typename T>
        static T Get(T begin, T end) {
            Y_ASSERT(begin != end);
            return --end;
        }
    };
}

namespace NTreePrivate {
    // To enable complex keys one should provide TraitsType with functions
    // bool EmptyKey(TraitsType::TKey key)
    // -- checks whether a key is empty (empty keys will not be added to children)
    Y_HAS_MEMBER(EmptyKey);

    // TraitsType::TKey StoreKey(TraitsType::TKey key, TMemoryPool& pool)
    // -- stores key on pool
    Y_HAS_MEMBER(StoreKey);

    // std::pair<TKeyIter, TKeyIter> KeyRange(const TraitsType::TKey* key)
    // -- return range for given key
    Y_HAS_MEMBER(KeyRange);

    // TraitsType::TKey ExtractKey(TKeyIter begin, TKeyIter end)
    // -- extract key from given range
    Y_HAS_MEMBER(ExtractKey);

    // bool AdvanceKey(TKeyIter& begin, TKeyIter& end, const TraitsType::TKey& key)
    // -- try to advance key range (begin, end) by some key, return true if key folds in range
    Y_HAS_MEMBER(AdvanceKey);

    template <typename TTree, bool Enabled>
    struct TComplexKeyBase {
        enum {
            IsEnabled = false
        };
        static bool IsEmpty(typename TTree::TKey) {
            return false;
        }
        static typename TTree::TKey Store(typename TTree::TKey key, TMemoryPool&) {
            return key;
        }
        static std::pair<typename TTree::TKeyIter, typename TTree::TKeyIter> Range(const typename TTree::TKey* key) {
            Y_ASSERT(key);
            return std::make_pair(key, key + 1);
        }
        template <typename TKeyIter>
        static typename TTree::TKey Extract(TKeyIter begin, TKeyIter end) {
            return *TTree::TDirection::Get(begin, end);
        }
        template <typename TKeyIter>
        static bool Advance(TKeyIter& begin, TKeyIter& end, typename TTree::TKey) {
            TTree::TDirection::Inc(begin, end);
            return true;
        }
    };

    template <typename TTree>
    struct TComplexKeyBase<TTree, true> {
        enum {
            IsEnabled = true
        };
        static bool IsEmpty(typename TTree::TKey key) {
            return TTree::TTraits::EmptyKey(key);
        }
        static typename TTree::TKey Store(typename TTree::TKey key, TMemoryPool& pool) {
            return TTree::TTraits::StoreKey(key, pool);
        }
        static std::pair<typename TTree::TKeyIter, typename TTree::TKeyIter> Range(const typename TTree::TKey* key) {
            return TTree::TTraits::KeyRange(key);
        }
        template <typename TKeyIter>
        static typename TTree::TKey Extract(TKeyIter begin, TKeyIter end) {
            return TTree::TTraits::ExtractKey(begin, end);
        }
        template <typename TKeyIter>
        static bool Advance(TKeyIter& begin, TKeyIter& end, typename TTree::TKey key) {
            return TTree::TTraits::AdvanceKey(begin, end, key);
        }
    };

    template <typename TTree>
    struct TComplexKey: public TComplexKeyBase<
                             TTree,
                             THasEmptyKey<typename TTree::TTraits>::value &&
                                 THasStoreKey<typename TTree::TTraits>::value &&
                                 THasKeyRange<typename TTree::TTraits>::value &&
                                 THasExtractKey<typename TTree::TTraits>::value &&
                                 THasAdvanceKey<typename TTree::TTraits>::value> {
    };

    // To enable partial matching one should provide TraitsType with functions
    // TraitsType::TKey GetPrefix(const TraitsType::TKey& a, const TraitsType::TKey& b)
    // -- return a's prefix, common for keys a & b
    Y_HAS_MEMBER(GetPrefix);

    // TraitsType::TKey GetSuffix(const TraitsType::TKey& a, const TraitsType::TKey& b)
    // -- return a's suffix for common prefix between a & b
    Y_HAS_MEMBER(GetSuffix);

    template <typename TTree, bool Enabled>
    struct TSplitKeysBase {
        enum {
            IsEnabled = false
        };
        static typename TTree::TKey GetPrefix(typename TTree::TKey a, typename TTree::TKey) {
            return a;
        }
        static typename TTree::TKey GetSuffix(typename TTree::TKey a, typename TTree::TKey) {
            return a;
        }
    };

    template <typename TTree>
    struct TSplitKeysBase<TTree, true> {
        enum {
            IsEnabled = true
        };
        static typename TTree::TKey GetPrefix(typename TTree::TKey a, typename TTree::TKey b) {
            return TTree::TTraits::GetPrefix(a, b);
        }
        static typename TTree::TKey GetSuffix(typename TTree::TKey a, typename TTree::TKey b) {
            return TTree::TTraits::GetSuffix(a, b);
        }
    };

    template <typename TTree>
    struct TSplitKeys: public TSplitKeysBase<
                            TTree,
                            THasGetPrefix<typename TTree::TTraits>::value &&
                                THasGetSuffix<typename TTree::TTraits>::value> {
    };
}

template <
    typename DataType,
    typename TraitsType,
    typename DirectionType = NDirections::TForward>
class TTreeBase {
public:
    typedef DataType TData;
    typedef TraitsType TTraits;
    typedef typename TTraits::TKey TKey;
    typedef typename TTraits::TKeyIter TKeyIter;
    typedef DirectionType TDirection;
    typedef TTreeBase<TData, TTraits, TDirection> TSelf;
    struct TRange;

public:
    TTreeBase(size_t size = 1u << 20)
        : PoolHolder(new TMemoryPool(size))
        , Pool(PoolHolder.Get())
        , Root(NewNode())
    {
        Root->Children = NewChildren();
    }

    // External pool
    TTreeBase(TMemoryPool* pool)
        : Pool(pool)
        , Root(NewNode())
    {
        Root->Children = NewChildren();
    }

    TTreeBase(const TTreeBase& tree)
        : PoolHolder(tree.PoolHolder.Get() ? new TMemoryPool(tree.Pool->MemoryAllocated()) : nullptr)
        , Pool(tree.Pool)
        , Root(CopyNode(tree.Root))
    {
    }

    // Returns non-zero pointer to data if corresponding path exists in tree
    const TData* Find(TKeyIter begin, TKeyIter end) const {
        TConstLocus path = Traverse(TRange(begin, end), GetRoot());
        if (path.Pos == begin && path.End == end)
            return &path.Node->Data;
        return nullptr;
    }

    template <typename TKeyRange>
    const TData* Find(const TKeyRange& r) const {
        return Find(r.begin(), r.end());
    }

    // Returns max found subrange and a pointer to corresponding data
    std::pair<std::pair<TKeyIter, TKeyIter>, const TData*> Path(TKeyIter begin, TKeyIter end) const {
        return Traverse(TRange(begin, end), GetRoot()).AsPairs();
    }

    template <typename TKeyRange>
    std::pair<std::pair<TKeyIter, TKeyIter>, const TData*> Path(const TKeyRange& r) const {
        return Path(r.begin(), r.end());
    }

    // Sets data. If no corresponding path exists in tree, adds a child to least node in path, possibly truncating range
    TData& At(TKeyIter begin, TKeyIter end) {
        return Locate(TRange(begin, end), GetRoot()).Node->Data;
    }

    template <typename TKeyRange>
    TData& At(const TKeyRange& r) {
        return At(r.begin(), r.end());
    }

    const TData& At(TKeyIter begin, TKeyIter end) const {
        const TData* data = Find(begin, end);
        return data ? *data : TData();
    }

    template <typename TKeyRange>
    const TData& At(const TKeyRange& r) const {
        return At(r.begin(), r.end());
    }

    static TKey GetKey(TKeyIter begin, TKeyIter end) {
        return NTreePrivate::TComplexKey<TSelf>::Extract(begin, end);
    }

    static TRange GetRange(const TKey* key) {
        return TRange(NTreePrivate::TComplexKey<TSelf>::Range(key));
    }

    // Implementation
public:
    // Keys range
    struct TRange {
        TKeyIter Pos;
        TKeyIter End;

        TRange(TKeyIter begin, TKeyIter end)
            : Pos(begin)
            , End(end)
        {
        }

        TRange(std::pair<TKeyIter, TKeyIter> p)
            : Pos(p.first)
            , End(p.second)
        {
        }

        TRange(const TRange&) = default;
        TRange& operator=(const TRange&) = default;

        TKey GetKey() const {
            // Begin != End
            return TSelf::GetKey(Pos, End);
        }

        std::pair<TKeyIter, TKeyIter> AsPair() const {
            return std::make_pair(Pos, End);
        }

        size_t Length() const {
            return End - Pos;
        }

        bool IsValid() const {
            return Pos != End;
        }
    };

protected:
    struct TNode;

    typedef typename TTraits::template TChildren<TNode> TChildrenOfNode;
    class TChildren: public TPoolable, public TChildrenOfNode {
    public:
        TChildren(TMemoryPool* pool)
            : TChildrenOfNode(pool)
        {
        }
    };

    // A tree node
    // Label is stored with each child in TTraits::TChildren<TDerived>, e.g. as a key in hash
    struct TNode: public TPoolable {
        TData Data;
        TChildren* Children;

        TNode(const TData& data)
            : Data(data)
            , Children(nullptr)
        {
        }
    };

    static TRange ComplementaryPrefix(TRange full, TRange suff) {
        if (std::is_same<TDirection, NDirections::TForward>::value)
            return TRange(full.Pos, full.End - suff.Length());
        else
            return TRange(full.Pos + suff.Length(), full.End);
    }

    static TRange ComplementarySuffix(TRange full, TRange pref) {
        if (std::is_same<TDirection, NDirections::TForward>::value)
            return TRange(full.Pos + pref.Length(), full.End);
        else
            return TRange(full.Pos, full.End - pref.Length());
    }

protected:
    // Locus - a node and a corresponding range
    template <bool Const>
    struct TLocusImpl: public TRange {
        typedef std::conditional_t<Const, const TNode*, TNode*> TNodePtr;

        // Pointer to last valid node found (searched node or parent node for unfound or partially matched child)
        TNodePtr Node;

        TLocusImpl(TNodePtr node, TRange r)
            : TRange(r)
            , Node(node)
        {
        }

        TLocusImpl(const TLocusImpl&) = default;
        TLocusImpl& operator=(const TLocusImpl&) = default;

        std::pair<std::pair<TKeyIter, TKeyIter>, const TData*> AsPairs() const {
            return std::make_pair(TRange::AsPair(), &(Node->Data));
        }
    };

    typedef TLocusImpl<false> TLocus;
    typedef TLocusImpl<true> TConstLocus;

    TNode* GetRoot() {
        return Root;
    }

    const TNode* GetRoot() const {
        return Root;
    }

    struct TLoci {
        TNode* Node; // Node, corresponding to given range (possibly new)
        TLocus Fork; // Branching locus in tree (with possibly new node) (locus range is a prefix of searched range)
        TLocus Base; // Parent locus to branching one in old tree (possibly the same) (locus range is a prefix of searched range)
        TLocus Part; // Old child node (possibly empty) of branching node in old tree (with path from branching node)

        TLoci(TNode* node, TLocus fork, TLocus base, TLocus part)
            : Node(node)
            , Fork(fork)
            , Base(base)
            , Part(part)
        {
        }
    };

    // Follows label sequence in tree, finds corresponding node or adds new node
    TLoci Locate(TRange range, TNode* base) {
        Y_ASSERT(base);
        if (!range.IsValid())
            return TLoci(base, TLocus(base, range), TLocus(base, range), TLocus(nullptr, range));

        TTraversed<false> dst(Traverse<false>(range, base));

        // Any returned node must be initialized
        Y_ASSERT(dst.Node);

        TNode* node = dst.Node;
        TLocus fork(dst.Node, ComplementaryPrefix(range, dst));
        if (dst.Pos != dst.End) {
            if (dst.Part.IsValid() && dst.Part.Node) {
                if (NTreePrivate::TSplitKeys<TSelf>::IsEnabled) {
                    // Handle partially matched keys if enabled in TTraits
                    std::pair<TLocus, TNode*> p = SplitPartiallyMatchedKeys(dst, dst.Part);
                    node = p.first.Node;
                    // New branching node with corresponding subrange
                    fork = TLocus(p.second, ComplementaryPrefix(range, p.first));
                } else {
                    node = dst.Part.Node;
                }
            } else {
                // A new leaf node is added
                if (!dst.Node->Children)
                    dst.Node->Children = NewChildren();
                node = AddNode(dst.Node->Children, dst.GetKey()).Node;
            }
        }
        return TLoci(node, fork, TLocus(dst.Node, ComplementaryPrefix(range, dst)), dst.Part);
    }

    // Follows label sequence in tree, returns path to the last matching node found
    TConstLocus Traverse(TRange range, const TNode* base) const {
        Y_ASSERT(base);
        if (!range.IsValid())
            return TConstLocus(base, range);

        TTraversed<true> dst(Traverse<true>(range, base));
        // Any returned node must be initialized
        Y_ASSERT(dst.Node);
        return TConstLocus(dst.Node, ComplementaryPrefix(range, dst));
    }

    template <bool Const>
    struct TTraversed: public TLocusImpl<Const> {
        typedef typename TLocusImpl<Const>::TNodePtr TNodePtr;

        TLocusImpl<Const> Part;

        TTraversed(TNodePtr node, TRange range)
            : TLocusImpl<Const>(node, range)
            , Part(nullptr, ComplementarySuffix(range, range))
        {
        }

        TTraversed(const TTraversed& t)
            : TLocusImpl<Const>(t)
            , Part(t.Part)
        {
        }

        TTraversed(TNodePtr node, TRange range, TLocusImpl<Const> part)
            : TLocusImpl<Const>(node, range)
            , Part(part)
        {
        }
    };

    // Traverses tree from given base node until first unmatched label in range found or until leaf reached
    // Returns matched node with empty range or node for which a child was not found with the rest of range (untraversed)
    template <bool Const>
    static TTraversed<Const> Traverse(TRange range, typename TTraversed<Const>::TNodePtr base) {
        typedef std::conditional_t<Const, typename TChildren::const_iterator, typename TChildren::iterator> TValueIter;
        TTraversed<Const> curr(base, range);

        while (true) {
            // Node must be initialized
            Y_ASSERT(curr.Node);
            if (curr.Pos == curr.End || !(curr.Node->Children)) {
                // Labels exhausted or leaf node reached
                break;
            }
            // Matching can be partial, meaning that find(*it) can return child, for which TTraits::Equal(label, *it) != 0,
            // (but TTraits::THashEqual()(label, *it) == true)
            // This is made to allow string labels indexing by first character
            TValueIter next(curr.Node->Children->find(curr.GetKey()));
            if (next == curr.Node->Children->end()) {
                // No corresponding child found
                break;
            }
            // Compare searched and found keys, and advance key iterator by found key if keys matched
            if (!NTreePrivate::TComplexKey<TSelf>::Advance(curr.Pos, curr.End, next->first)) {
                // Partial matching: keep pointer to partially matched key
                // Return partially matched node and its key
                curr.Part = TLocusImpl<Const>(next->second, GetRange(&next->first));
                break;
            }
            curr.Node = next->second;
        }
        return curr;
    }

    // Split partially matched key ranges
    // Return node, corresponding to given range with its path from branching node and the branching node (possibly same)
    std::pair<TLocus, TNode*> SplitPartiallyMatchedKeys(TLocus curr, TLocus part) {
        // Create new intermediate node
        TNode* node = NewNode();
        // Create children for intermediate node
        node->Children = NewChildren();

        // Add {old suffix -> partially matched child node} to inermediate node
        // Key is already stored on Pool, so pointers in selected suffix will remain valid
        node->Children->insert(std::make_pair(NTreePrivate::TSplitKeys<TSelf>::GetSuffix(part.GetKey(), curr.GetKey()), part.Node));

        // Replace {partially matched child with prefix -> intermediate node} for current node
        curr.Node->Children->erase(part.GetKey());

        // Key is already stored on pool, so pointers in selected prefix will remain valid
        TKey prefix(NTreePrivate::TSplitKeys<TSelf>::GetPrefix(part.GetKey(), curr.GetKey()));
        curr.Node->Children->insert(std::make_pair(prefix, node));

        // Add {new suffix -> new node} to intermediate node
        TKey suffix = NTreePrivate::TSplitKeys<TSelf>::GetSuffix(curr.GetKey(), part.GetKey());
        if (!NTreePrivate::TComplexKey<TSelf>::IsEmpty(suffix)) {
            // Suffix is taken for temporary range and will be stored on Pool
            return std::make_pair(AddNode(node->Children, suffix), node);
        }
        return std::make_pair(TLocus(node, ComplementarySuffix(curr, curr)), node);
    }

    // Node creation, adding and copying
    TNode* NewNode() {
        return new (*Pool) TNode(TData());
    }

    TChildren* NewChildren() {
        return new (*Pool) TChildren(Pool);
    }

    TLocus AddNode(TChildren* to, const TKey& key) {
        TNode* node = NewNode();
        typename TChildren::iterator i = (to->insert(std::make_pair(NTreePrivate::TComplexKey<TSelf>::Store(key, *Pool), node))).first;
        return TLocus(i->second, GetRange(&i->first));
    }

    TNode* CopyNode(const TNode* node) {
        if (!node)
            return nullptr;
        TNode* copy = NewNode();
        copy->Data = node->Data;
        if (node->Children) {
            copy->Children = NewChildren();
            for (typename TChildren::const_iterator i = node->Children->begin(); i != node->Children->end(); ++i)
                copy->Children->insert(std::make_pair(NTreePrivate::TComplexKey<TSelf>::Store(i->first, *Pool), CopyNode(i->second)));
        }
        return copy;
    }

private:
    THolder<TMemoryPool> PoolHolder;
    TMemoryPool* Pool;
    TNode* Root;
};
