#pragma once

#include "tree.h"

namespace NTriePrivate {
    // Key of a trie is a character range
    template <typename TCharType, typename DirectionType>
    struct TTrieKey {
        typedef DirectionType TDirection;

        const TCharType* Begin;
        const TCharType* End;

        TTrieKey(const TCharType* b = nullptr, const TCharType* e = nullptr)
            : Begin(b)
            , End(e)
        {
        }

        bool IsValid() const {
            return Begin && End && Begin != End;
        }

        TTrieKey& operator++() {
            TDirection::Inc(Begin, End);
            return *this;
        }

        TCharType operator*() const {
            return *TDirection::Get(Begin, End);
        }
    };

    template <typename CharType, typename DirectionType>
    struct TBaseTrieKeyTraits {
        typedef TTrieKey<CharType, DirectionType> TKey;

        static bool EmptyKey(TKey key) {
            return key.Begin == key.End;
        }

        static TKey StoreKey(TKey key, TMemoryPool& pool) {
            size_t l = key.End - key.Begin;
            const CharType* p = pool.Append(key.Begin, l);
            return TKey(p, p + l);
        }

        static std::pair<const CharType*, const CharType*> KeyRange(const TKey* key) {
            Y_ASSERT(key);
            return std::make_pair(key->Begin, key->End);
        }

        static TKey ExtractKey(const CharType* b, const CharType* e) {
            return TKey(b, e);
        }

        static TKey GetPrefix(TKey a, TKey b) {
            TKey p(a), q(b);
            while (p.IsValid() && q.IsValid() && *p == *q) {
                ++p;
                ++q;
            }
            // Left is common part for forward (fixed 'end') direction, right part is common for backward (fixed 'begin') direction
            return a.Begin != p.Begin ? TKey(a.Begin, p.Begin) : TKey(p.End, a.End);
        }

        static TKey GetSuffix(TKey a, TKey b) {
            while (a.IsValid() && b.IsValid() && *a == *b) {
                ++a;
                ++b;
            }
            return a;
        }
    };

    template <typename CharType, typename DirectionType>
    struct TTrieKeyTraits: public TBaseTrieKeyTraits<CharType, DirectionType> {
        typedef typename TBaseTrieKeyTraits<CharType, DirectionType>::TKey TKey;
        typedef DirectionType TDirection;

        static bool AdvanceKey(const CharType*& begin, const CharType*& end, TKey key) {
            TKey a(begin, end), b(key);
            while (a.IsValid() && b.IsValid() && *a == *b) {
                ++a;
                ++b;
            }
            if (!b.IsValid()) {
                begin = a.Begin;
                end = a.End;
                return true;
            }
            return false;
        }
    };

    // Small specialization for 'char'
    template <typename DirectionType>
    struct TTrieKeyTraits<char, DirectionType>: public TBaseTrieKeyTraits<char, DirectionType> {
        typedef TBaseTrieKeyTraits<char, DirectionType> TBase;
        typedef typename TBase::TKey TKey;

        static bool AdvanceKey(const char*& begin, const char*& end, TKey key) {
            size_t m = end - begin, n = key.End - key.Begin;
            if (m < n || memcmp(std::is_same<DirectionType, NDirections::TForward>::value ? begin : end - n, key.Begin, n)) {
                return false;
            }
            if (std::is_same<DirectionType, NDirections::TForward>::value)
                begin += n;
            else
                end -= n;
            return true;
        }
    };

    // Array traits for TTrieKey, using given ArrayTraitsType
    template <typename ArrayTraitsType, typename DirectionType>
    struct TTrieKeyArrayTraits {
        typedef TTrieKey<typename ArrayTraitsType::TKey, DirectionType> TKey;

        enum {
            // Number of letters in alphabet
            Size = ArrayTraitsType::Size,
        };

        // Key initialization check
        static bool Initialized(TKey key) {
            return key.IsValid();
        }

        // A function which maps a character to a number in range 0 .. Size - 1.
        static size_t Index(TKey key) {
            return ArrayTraitsType::Index(*key);
        }
    };

    // Hash function for TTrieKey, taking first (in some direction) character of key range
    template <typename TKey, typename THash>
    struct TTrieKeyHashFunction {
        size_t operator()(TKey key) const {
            return THash()(*key);
        }
    };

    // Hash key equality check for TTrieKey, comparing first (in some direction) characters of key ranges
    template <typename TKey, typename TEqual>
    struct TTrieKeyHashEqual {
        bool operator()(TKey a, TKey b) const {
            return TEqual()(*a, *b);
        }
    };
}

// Traits for trie with node children array
template <
    typename ArrayTraitsType,
    typename DirectionType = NDirections::TForward>
struct TBaseArrayTrieTraits: public NTriePrivate::TTrieKeyTraits<typename ArrayTraitsType::TKey, DirectionType>,            // Traits, handling trie keys
                              public TBaseArrayTreeTraits<NTriePrivate::TTrieKeyArrayTraits<ArrayTraitsType, DirectionType>> // Traits, handling children array structure tor trie
{
    typedef typename NTriePrivate::TTrieKey<typename ArrayTraitsType::TKey, DirectionType> TKey;
    typedef const typename ArrayTraitsType::TKey* TKeyIter;
};

using TCharArrayTrieTraits = TBaseArrayTrieTraits<TCharArrayTraits>;

// Traits for trie with node children hash
template <
    typename CharType,
    typename DirectionType = NDirections::TForward,
    typename HashType = THash<CharType>,
    typename EqualType = TEqualTo<CharType>>
struct TBaseHashTrieTraits: public NTriePrivate::TTrieKeyTraits<CharType, DirectionType>, // Traits, handling trie keys
                             public TBaseHashTreeTraits<
                                 typename NTriePrivate::TTrieKey<CharType, DirectionType>,
                                 NTriePrivate::TTrieKeyHashFunction<typename NTriePrivate::TTrieKey<CharType, DirectionType>, HashType>,
                                 NTriePrivate::TTrieKeyHashEqual<typename NTriePrivate::TTrieKey<CharType, DirectionType>, EqualType>> // Traits, handling children hash structure for trie
{
    typedef typename NTriePrivate::TTrieKey<CharType, DirectionType> TKey;
    typedef const CharType* TKeyIter;
};
