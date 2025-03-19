#pragma once

#include <util/str_stl.h>
#include <util/generic/hash_primes.h>
#include <util/generic/vector.h>

// Class for caching calculations
// On collision rewrites with new data
template <typename T_Key, typename T_Value>
class THashCache {
public:
    explicit THashCache(size_t size = 0) { if (size > 0) Reset(size); }
    void Reset(size_t size); // clear cache and set size
    void Reset(); // clear cache and keep inintial size

    bool IsInitialized() const { return !hashTable.empty(); }

    bool Lookup(const T_Key&, T_Value&) const;
    bool LookupAndReserve(const T_Key& key, T_Value*& resultPtr); // result MUST be written or garbage next time
    void Set(const T_Key&, const T_Value&);

private:
    struct TEntry {
        T_Key Key;
        T_Value Value;
        bool IsInitialized;

        TEntry() : IsInitialized(false) {}
    };
    TVector<TEntry> hashTable;
};

template <typename T_Key, typename T_Value>
void THashCache<T_Key, T_Value>::Reset(size_t size) {
    size = HashBucketCount(size);
    hashTable.clear();
    hashTable.resize(size);
}

template <typename T_Key, typename T_Value>
void THashCache<T_Key, T_Value>::Reset() {
    size_t size = hashTable.size();
    hashTable.clear();
    hashTable.resize(size);
}

template <typename T_Key, typename Value>
inline bool THashCache<T_Key, Value>::Lookup(const T_Key& key, Value& result) const {
    if (hashTable.empty()) {
        return false;
    }
    size_t index = THash<T_Key>()(key) % hashTable.size();
    if (!hashTable[index].IsInitialized || !TEqualTo<T_Key>()(hashTable[index].Key, key)) {
        return false;
    }
    result = hashTable[index].Value;
    return true;
}

template <typename T_Key, typename T_Value>
inline bool THashCache<T_Key, T_Value>::LookupAndReserve(const T_Key& key, T_Value*& result) {
    Y_ASSERT(!hashTable.empty());
    size_t index = THash<T_Key>()(key) % hashTable.size();
    result = &hashTable[index].Value;
    if (!hashTable[index].IsInitialized || !TEqualTo<T_Key>()(hashTable[index].Key, key)) {
        hashTable[index].Key = key;
        hashTable[index].IsInitialized = true;
        return false;
    }
    return true;
}

template <typename T_Key, typename T_Value>
inline void THashCache<T_Key, T_Value>::Set(const T_Key& key, const T_Value& result) {
    Y_ASSERT(!hashTable.empty());
    size_t index = THash<T_Key>()(key) % hashTable.size();
    hashTable[index].Key = key;
    hashTable[index].Value = result;
    hashTable[index].IsInitialized = true;
}


