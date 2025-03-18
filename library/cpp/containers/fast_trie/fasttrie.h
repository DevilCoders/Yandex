#pragma once

/*************************************************************************
 * TFastTrie ( = prefix tree) - a fast but memory hungry map
 *                         of strings to anything.
 *
 * some notes for the time of refactoring:
 *  https://beta.wiki.yandex-team.ru/users/alyashko/fasttriewishes/
 *
 *************************************************************************/

#include <util/system/defaults.h>
#include <util/system/compat.h>
#include <util/generic/singleton.h>
#include <util/generic/ptr.h>
#include <util/memory/alloc.h>
#include <util/memory/pool.h>
#include <util/charset/unidata.h>
#include <utility>

/**
 * Two ways to iterate a range
 */

namespace NTrieDirections { // FTCS :(
    template <class _TItem, class _TIter>
    struct TForward {
        typedef _TItem TItem;
        typedef _TIter TIter;
        static void Inc(TIter& begin, TIter& end) {
            if (begin != end)
                ++begin;
        }

        static TItem Get(TIter begin, TIter end) {
            if (begin != end)
                return *begin;
            else
                return TItem();
        }
    };

    template <class _TItem, class _TIter>
    struct TBackward {
        typedef _TIter TIter;
        typedef _TItem TItem;
        static void Inc(TIter& begin, TIter& end) {
            if (begin != end)
                --end;
        }

        static TItem Get(TIter begin, TIter end) {
            if (begin != end) {
                TIter p = end;
                return *--p;
            } else
                return TItem();
        }
    };
}

/**
 * A default traits for TFastTrie.
 * Customizing it may seriously decrease amount of required memory.
 */

template <typename TTraits, typename TDirection>
struct TTrieUtils {
    static inline bool IsPrefixOf(typename TDirection::TIter begin1, typename TDirection::TIter end1, typename TDirection::TIter begin2, typename TDirection::TIter end2) {
        while (begin1 != end1 && begin2 != end2) {
            if (TTraits::Index(TDirection::Get(begin1, end1)) != TTraits::Index(TDirection::Get(begin2, end2))) {
                return false;
            }
            TDirection::Inc(begin1, end1);
            TDirection::Inc(begin2, end2);
        }
        return (begin1 == end1);
    }
};

struct TTrieTraits {
    typedef char CharType;

    enum {
        /// Number of letters in alphabet
        Size = 256,
    };

    /// A function which maps a character to a number in range 0 .. Size-1.
    static inline size_t Index(CharType c) {
        return static_cast<unsigned char>(c);
    }

    static inline bool Equal(const char* lhs, const char* rhs, size_t l) {
        return !strncmp(lhs, rhs, l);
    }
};

template <
    typename DataT,
    typename TraitsT = TTrieTraits,
    typename DirectionT = NTrieDirections::TForward<typename TraitsT::CharType, const typename TraitsT::CharType*>>
class TFastTrie {
public:
    typedef DataT DataType;
    typedef TraitsT TraitsType;
    typedef typename TraitsType::CharType CharType;
    typedef DirectionT TDirection;

public:
    TFastTrie(size_t pool_size = 1u << 20)
        : Pool(new TMemoryPool(pool_size))
        , Root(nullptr)
    {
    }
    TFastTrie(const TFastTrie& trie)
        : Pool(new TMemoryPool(trie.Pool->MemoryAllocated()))
        , Root(CopyNode(trie.Root))
    {
    }
    TFastTrie& operator=(const TFastTrie& trie);

    /**
     * Returns the data for the given key
     * or NULL if given key was not inserted into the tree.
     */

    const DataType* Find(const CharType* begin, const CharType* end) const;

    const DataType* FindByPrefix(const CharType* begin, const CharType* end) const;

    template <typename RangeType>
    const DataType* Find(const RangeType& r) const {
        return Find(r.begin(), r.end());
    }

    /**
     * Returns key's prefix(suffix) length in trie and checks whether key points to any data
     */

    std::pair<size_t, bool> FindPath(const CharType* begin, const CharType* end) const;

    template <typename RangeType>
    std::pair<size_t, bool> FindPath(const RangeType& r) const {
        return FindPath(r.begin(), r.end());
    }

    size_t Path(const CharType* begin, const CharType* end) const {
        return FindPath(begin, end).first;
    }

    template <typename RangeType>
    size_t Path(const RangeType& r) const {
        return Path(r.begin(), r.end());
    }

    /**
     * Returns a reference to the data corresponding the given key,
     * default-constructing it if neccessary.
     */

    DataType& At(const CharType* begin, const CharType* end);

    template <typename RangeType>
    DataType& At(const RangeType& r) {
        return At(r.begin(), r.end());
    }

    const DataType& At(const CharType* begin, const CharType* end) const;

    template <typename RangeType>
    const DataType& At(const RangeType& r) const {
        return At(r.begin(), r.end());
    }

    template <typename RangeType>
    DataType& operator[](const RangeType& r) {
        return At(r);
    }

    template <typename RangeType>
    const DataType& operator[](const RangeType& r) const {
        return At(r);
    }

    /**
     * Inserts the pair (key, data) into the tree, overwriting
     * previous value if any.
     * Returns true   if the value was inserted,
     *         false  if the value has overwritten previous value.
     */

    auto Insert(const CharType* begin, const CharType* end, const DataType& data)
        -> std::enable_if_t<std::is_copy_assignable_v<DataType>, bool>;

    auto Insert(const CharType* begin, const CharType* end, DataType&& data)
        -> std::enable_if_t<std::is_move_assignable_v<DataType>, bool>;

    template <typename RangeType>
    bool Insert(const RangeType& r, const DataType& data) {
        return Insert(r.begin(), r.end(), data);
    }

    template <typename RangeType>
    bool Insert(const RangeType& r, DataType&& data) {
        return Insert(r.begin(), r.end(), data);
    }

private:
    /// A base node.
    struct TNode {
        // Yeah, I know that type codes are ugly and generally should
        // be replaced by inheritance, but I want this thing run REALLY fast
        enum EType {
            Leaf = 1,
            Fork = 2
        };

        EType Type;

        TNode(EType type)
            : Type(type)
        {
        }

        TNode& operator=(const TNode&); // non-assignable (but still copyable)
    };

    /// A leaf.
    struct TLeaf: public TNode, public TPoolable {
        /// The tail of the key, unconsumed by parents
        const CharType* Begin;
        const CharType* End;

        /// Value
        DataType Data;

        /// Constructs a leaf from the given data
        TLeaf(const CharType* begin, const CharType* end, const DataType& data = DataType())
            : TNode(TNode::Leaf)
            , Begin(begin)
            , End(end)
            , Data(data)
        {
        }
    };

    /// A node containing other nodes
    struct TFork: public TNode, public TPoolable {
        /// A leaf for the key at the current level (if any)
        TNode* Self;

        /// Leaves for the next level
        TNode* Children[TraitsType::Size];

        TFork()
            : TNode(TNode::Fork)
        {
            Self = nullptr;
            memset(Children, 0, sizeof(Children));
        }
    };

private:
    /// Drivers
    TLeaf*& Place(const CharType*& begin, const CharType*& end);

    TLeaf* GrowLeaf(const CharType* begin, const CharType* end) {
        const size_t l = end - begin, bl = l * sizeof(CharType);
        void* p = Pool->Allocate(bl);
        memcpy(p, (const void*)begin, bl);
        return new (*Pool) TLeaf((const CharType*)p, (const CharType*)p + l);
    }

    TFork* GrowFork() {
        return new (*Pool) TFork;
    }

    TNode* CopyNode(const TNode* node) {
        if (!node)
            return nullptr;
        if (node->Type == TNode::Leaf) {
            const TLeaf* from = static_cast<const TLeaf*>(node);
            TLeaf* to = GrowLeaf(from->Begin, from->End);
            to->Data = from->Data;
            return static_cast<TNode*>(to);
        } else if (node->Type == TNode::Fork) {
            const TFork* from = static_cast<const TFork*>(node);
            TFork* to = GrowFork();
            to->Self = CopyNode(from->Self);
            for (size_t i = 0; i < TraitsType::Size; ++i)
                to->Children[i] = CopyNode(from->Children[i]);
            return static_cast<TNode*>(to);
        } else
            Y_ASSERT(!"Strange node type encountered");
        return nullptr;
    }

private:
    THolder<TMemoryPool> Pool;
    TNode* Root;
};

struct TCITextTrieTraits {
    typedef char CharType;

    enum {
        Size = ('z' - 'a' + 1) + ('9' - '0' + 1) + 1,
        Junk = Size - 1
    };

    static size_t Index(char c) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            return ToLower(c) - 'a';
        else if (c >= '0' && c <= '9')
            return (c - '0') + ('z' - 'a' + 1);
        else
            return Junk;
    }

    static bool Equal(const char* lhs, const char* rhs, size_t l) {
        return !strnicmp(lhs, rhs, l);
    }

    static size_t Length(const std::pair<const char*, const char*>& r) {
        return r.second - r.first;
    }
};

template <class TData>
class TCIFastTextTrie: public TFastTrie<TData, TCITextTrieTraits, NTrieDirections::TForward<char, const char*>> {
public:
    typedef TData DataType;
    typedef TCITextTrieTraits TraitsType;

    TCIFastTextTrie(size_t pool_size = 1u << 20)
        : TFastTrie<TData, TCITextTrieTraits>(pool_size)
    {
    }
};

// ===================================================================================
// Implementation
// ===================================================================================

template <typename D, typename T, typename W>
inline const D* TFastTrie<D, T, W>::Find(const typename T::CharType* begin, const typename T::CharType* end) const {
    const TNode* node = Root;
    while (node && node->Type == TNode::Fork && begin != end) {
        node = static_cast<const TFork*>(node)->Children[TraitsType::Index(TDirection::Get(begin, end))];
        TDirection::Inc(begin, end);
    }

    if (node && node->Type == TNode::Fork && begin == end)
        node = static_cast<const TFork*>(node)->Self;

    Y_ASSERT(!(node && node->Type != TNode::Leaf) && "TFastTrie::Find(): stopped at non-leaf node");

    const TLeaf* leaf = static_cast<const TLeaf*>(node);
    if (leaf && ((leaf->End - leaf->Begin == end - begin) && (begin == end || TraitsType::Equal(leaf->Begin, begin, end - begin))))
        return &leaf->Data;
    else
        return nullptr;
}

/*Find the longest key being a prefix of the given string:
    Insert("abcde", 1);
    *FindByPrefix("abcde") == 1;
    *FindByPrefix("abcdefgh") == 1;
    FindByPrefix("abc") == NULL;

*/
template <typename D, typename T, typename W>
inline const D* TFastTrie<D, T, W>::FindByPrefix(const typename T::CharType* begin, const typename T::CharType* end) const {
    const TNode* node = Root;
    const D* result = nullptr;
    if (node && node->Type == TNode::Leaf) {
        const TLeaf* leaf = static_cast<const TLeaf*>(node);
        if (TTrieUtils<T, W>::IsPrefixOf(leaf->Begin, leaf->End, begin, end)) {
            result = &leaf->Data;
        }
    }
    while (node && node->Type == TNode::Fork && begin != end) {
        node = static_cast<const TFork*>(node)->Children[TraitsType::Index(TDirection::Get(begin, end))];
        TDirection::Inc(begin, end);
        if (node != nullptr) {
            const TLeaf* leaf;
            if (node->Type == TNode::Fork) {
                leaf = static_cast<const TLeaf*>(static_cast<const TFork*>(node)->Self);
            } else {
                leaf = static_cast<const TLeaf*>(node);
            }
            if (leaf && TTrieUtils<T, W>::IsPrefixOf(leaf->Begin, leaf->End, begin, end)) {
                result = &leaf->Data;
            }
        }
    }

    return result;
}

template <typename D, typename T, typename W>
inline std::pair<size_t, bool> TFastTrie<D, T, W>::FindPath(const typename T::CharType* begin, const typename T::CharType* end) const {
    if (begin == end)
        return std::make_pair(0, false);
    const TNode* node = Root;
    const CharType *b = begin, *e = end;
    while (node && node->Type == TNode::Fork && b != e) {
        node = static_cast<const TFork*>(node)->Children[TraitsType::Index(TDirection::Get(b, e))];
        if (node)
            TDirection::Inc(b, e);
    }
    bool found = false;
    if (node && node->Type == TNode::Fork)
        found = (b == e) && static_cast<const TFork*>(node)->Self;

    if (node && node->Type == TNode::Leaf) {
        const TLeaf* leaf = static_cast<const TLeaf*>(node);
        const CharType *lb = leaf->Begin, *le = leaf->End;
        while (b != e && lb != le) {
            if (TraitsType::Index(TDirection::Get(b, e)) != TraitsType::Index(TDirection::Get(lb, le)))
                break;
            TDirection::Inc(b, e);
            TDirection::Inc(lb, le);
        }
        found = (b == e) && (lb == le);
    }
    return (b == begin && e == end) ? std::make_pair((size_t)0, found) : (b != begin ? std::make_pair((size_t)(b - begin), found) : std::make_pair((size_t)(end - e), found));
}

/**
 * A helper function which selects a location
 * corresponding to the given key, creating
 * neccessary middle nodes and the leaf if needed.
 */
template <typename D, typename T, typename W>
inline typename TFastTrie<D, T, W>::TLeaf*& TFastTrie<D, T, W>::Place(const typename T::CharType*& begin, const typename T::CharType*& end) {
    TNode** node = &Root;
    while (true) {
        // Traverse the tree while we see middle nodes and we have letters in key left
        while (*node && (*node)->Type == TNode::Fork && begin != end) {
            node = &static_cast<TFork*>(*node)->Children[TraitsType::Index(TDirection::Get(begin, end))];
            TDirection::Inc(begin, end);
        }

        TLeaf** leaf = reinterpret_cast<TLeaf**>(node);

        if (
            // Found an empty placeholder for the new node, or...
            !*leaf || (

                          // ...found a leaf mathing the seen prefix and...
                          (*leaf)->Type == TNode::Leaf && (
                                                              // ...key tail matches given tail
                                                              ((*leaf)->End - (*leaf)->Begin == end - begin) && (end == begin || TraitsType::Equal((*leaf)->Begin, begin, end - begin)))))
            return *leaf;

        if ((*node)->Type == TNode::Fork && begin == end) {
            // We stopped at fork node and have eaten all the key
            node = &((static_cast<TFork*>(*node))->Self);
            leaf = reinterpret_cast<TLeaf**>(node);
            return *leaf;
        }

        if ((*node)->Type == TNode::Leaf) {
            // We stopped at leaf node, but it does not match given key,
            // prepend that leaf node with a fork node...
            TFork* fork = GrowFork();
            if ((*leaf)->Begin != (*leaf)->End) {
                fork->Children[TraitsType::Index(TDirection::Get((*leaf)->Begin, (*leaf)->End))] = *leaf;
                TDirection::Inc((*leaf)->Begin, (*leaf)->End);
            } else
                fork->Self = *leaf;
            *node = static_cast<TNode*>(fork);
            // ...and go on to the next letter
        }
    }
}

template <typename D, typename T, typename W>
inline D& TFastTrie<D, T, W>::At(const typename T::CharType* begin, const typename T::CharType* end) {
    TLeaf*& leaf = Place(begin, end);
    if (!leaf)
        leaf = GrowLeaf(begin, end);
    return leaf->Data;
}

template <typename D, typename T, typename W>
inline const D& TFastTrie<D, T, W>::At(const typename T::CharType* begin, const typename T::CharType* end) const {
    const DataType* ptr = Find(begin, end);
    if (ptr)
        return *ptr;
    else
        return Default<DataType>();
}

template <typename D, typename T, typename W>
inline auto TFastTrie<D, T, W>::Insert(const typename T::CharType* begin, const typename T::CharType* end, const D& data)
    -> std::enable_if_t<std::is_copy_assignable_v<D>, bool>
{
    TLeaf*& leaf = Place(begin, end);
    if (!leaf) {
        leaf = GrowLeaf(begin, end);
        leaf->Data = data;
        return true;
    } else {
        leaf->Data = data;
        return false;
    }
}

template <typename D, typename T, typename W>
inline auto TFastTrie<D, T, W>::Insert(const typename T::CharType* begin, const typename T::CharType* end, D&& data)
    -> std::enable_if_t<std::is_move_assignable_v<D>, bool>
{
    TLeaf*& leaf = Place(begin, end);
    if (!leaf) {
        leaf = GrowLeaf(begin, end);
        leaf->Data = std::move(data);
        return true;
    } else {
        leaf->Data = std::move(data);
        return false;
    }
}

/// Assignment operator.
template <typename D, typename T, typename W>
TFastTrie<D, T, W>& TFastTrie<D, T, W>::operator=(const TFastTrie<D, T, W>& tree) {
    if (this != &tree) {
        TFastTrie<D, T, W> treecopy(tree);
        Pool.Swap(treecopy.Pool);
        DoSwap(Root, treecopy.Root);
    }
    return *this;
}
