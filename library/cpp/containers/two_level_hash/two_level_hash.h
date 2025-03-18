#pragma once

#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>

template <
    class TKey,
    class TValue,
    size_t NumBuckets = 256,
    size_t NumTries = 3,
    class THasher = THash<TKey>>
class TTwoLevelHash: public TNonCopyable {
protected:
    enum EState {
        S_EMPTY = 0,
        S_SINGLE_ITEM = 1,
    };

    typedef THashMap<TKey, TValue, THasher> THash;
    typedef typename THash::const_iterator THashIter;

    /* Note on field order: items that are accessed most go first. This is cache-friendly. */

    THasher Hasher;
    size_t UsedItems;
    THash Hash;
    ui8 States[NumBuckets];
    alignas(TKey) unsigned char Keys[sizeof(TKey) * NumBuckets];
    alignas(TValue) unsigned char Vals[sizeof(TValue) * NumBuckets];

protected:
    inline size_t GetHash(const TKey& key) const {
        return Hasher(key);
    }

    inline size_t GetBucket(size_t keyHash, size_t offset) const {
        return (keyHash + offset) % NumBuckets;
    }

    inline const TKey* GetKeyPtr(size_t index) const {
        return reinterpret_cast<const TKey*>(Keys) + index;
    }

    inline TKey* GetKeyPtr(size_t index) {
        return reinterpret_cast<TKey*>(Keys) + index;
    }

    inline const TValue* GetValPtr(size_t index) const {
        return reinterpret_cast<const TValue*>(Vals) + index;
    }

    inline TValue* GetValPtr(size_t index) {
        return reinterpret_cast<TValue*>(Vals) + index;
    }

public:
    typedef TKey TKeyType;
    typedef TValue TValueType;

    static const size_t FIRST_LEVEL_SIZE = NumBuckets;
    static const size_t NUM_TRIES = NumTries;

public:
    class TConstIterator {
    private:
        const TTwoLevelHash* Parent;
        size_t Index;
        THashIter HashIter;

    public:
        TConstIterator(const TTwoLevelHash* parent, const THashIter& begin)
            : Parent(parent)
            , Index(0)
            , HashIter(begin)
        {
            for (Index = 0; Index < NumBuckets; ++Index) {
                if (parent->States[Index] != S_EMPTY) {
                    break;
                }
            }
        }

        TConstIterator(const THashIter& end)
            : Parent(nullptr)
            , Index(NumBuckets)
            , HashIter(end)
        {
        }

        const TKey& First() const {
            if (Index < NumBuckets) {
                return *Parent->GetKeyPtr(Index);
            } else {
                return HashIter->first;
            }
        }

        const TValue& Second() const {
            if (Index < NumBuckets) {
                return *Parent->GetValPtr(Index);
            } else {
                return HashIter->second;
            }
        }

        void operator++() {
            if (Index < NumBuckets) {
                ++Index;
                for (; Index < NumBuckets; ++Index) {
                    if (Parent->States[Index] != S_EMPTY) {
                        return;
                    }
                }
            } else {
                ++HashIter;
            }
        }

        bool operator==(const TConstIterator& other) const {
            return (Index == other.Index && HashIter == other.HashIter);
        }

        bool operator!=(const TConstIterator& other) const {
            return (Index != other.Index || HashIter != other.HashIter);
        }
    };

    friend class TConstIterator;
    typedef TConstIterator const_iterator;

    TTwoLevelHash() {
        MarkFirstLevelClear();
    }

    ~TTwoLevelHash() {
        ClearFirstLevel();
        /* Second level will clear itself automatically. */
    }

    void Clear() {
        ClearFirstLevel();
        THash().swap(Hash); // faster then clear().
    }

    size_t Size() const {
        return UsedItems + Hash.size();
    }

    const TValue* FindPtr(const TKey& key) const {
        const size_t keyHash = GetHash(key);
        size_t j = 0;
        for (size_t i = 0; i < NumTries; ++i) {
            const size_t bucket = GetBucket(keyHash, j);

            if (ui8(S_EMPTY) == States[bucket]) {
                return nullptr;
            } else if (*GetKeyPtr(bucket) == key) {
                return GetValPtr(bucket);
            }
            j += i + 1;
        }

        const THashIter it = Hash.find(key);
        if (Hash.end() == it) {
            return nullptr;
        }

        return &(it->second);
    }

    TConstIterator Begin() const {
        return TConstIterator(this, Hash.begin());
    }

    TConstIterator End() const {
        return TConstIterator(Hash.end());
    }

    TValue& operator[](const TKey& key) {
        const size_t keyHash = GetHash(key);
        size_t j = 0;
        for (size_t i = 0; i < NumTries; ++i) {
            const size_t bucket = GetBucket(keyHash, j);

            TKey& tKey = *GetKeyPtr(bucket);
            ui8& state = States[bucket];

            if (ui8(S_EMPTY) == state) {
                ++UsedItems;
                state = ui8(S_SINGLE_ITEM);

                TValue& tVal = *GetValPtr(bucket);

                /* That's placement new, just calls copy constructor. */
                new (&tKey) TKey(key);
                new (&tVal) TValue();

                return tVal;
            } else if (key == tKey) {
                return *GetValPtr(bucket);
            }
            j += i + 1;
        }

        return Hash[key];
    }

private:
    void ClearFirstLevel() {
        /* Note that gcc is smart enough to optimize away the whole loop in case
         * both TKey and TValue are trivially destructible. */
        for (size_t i = 0; i < NumBuckets; ++i) {
            if (States[i] != S_EMPTY) {
                GetKeyPtr(i)->~TKey();
                GetValPtr(i)->~TValue();
            }
        }

        MarkFirstLevelClear();
    }

    void MarkFirstLevelClear() {
        memset(States, S_EMPTY, sizeof(States));
        UsedItems = 0;
    }
};
