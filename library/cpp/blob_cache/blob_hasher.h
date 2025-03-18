#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/string/cast.h>
#include <util/memory/blob.h>

namespace NBlobHasherPrivate {
    template <typename TKey, size_t RetryCount>
    class THashPair {
    public:
        TKey Key;
        TBlob Value;

    public:
        inline bool GetLock() {
            for (size_t i = 0; i < RetryCount; ++i) {
                if (AtomicCas(&Lock, 1, 0)) {
                    return true;
                }
            }
            return false;
        }
        inline void ReleaseLock() {
            AtomicSet(Lock, 0);
        }

    private:
        TAtomic Lock = 0;
    };

}

template <typename TKey>
class TBlobHasher {
public:
    enum {
        DefaultSize = 1000 * 1000,
        RetryCount = 100
    };
    using THashPair = NBlobHasherPrivate::THashPair<TKey, RetryCount>;

public:
    TBlobHasher()
        : Hash(DefaultSize)
    {
    }
    TBlobHasher(size_t size)
        : Hash(size)
    {
    }
    bool SetBlobOrFail(const TKey& key, const TBlob& value) {
        THashPair& hashPair = GetHashPair(key);
        if (hashPair.GetLock()) {
            hashPair.Key = key;
            hashPair.Value = value;
            hashPair.ReleaseLock();
            return true;
        }
        return false;
    }
    bool GetBlobOrFail(const TKey& key, TBlob& value) {
        THashPair& hashPair = GetHashPair(key);
        TEqualTo<TKey> equalTo;
        if (hashPair.GetLock()) {
            if (equalTo(hashPair.Key, key)) {
                value = hashPair.Value;
                hashPair.ReleaseLock();
                return true;
            }
            hashPair.ReleaseLock();
        }
        return false;
    }

private:
    inline THashPair& GetHashPair(const TKey& key) {
        THash<TKey> hash;
        return Hash[hash(key) % Hash.size()];
    }

private:
    TVector<THashPair> Hash;
};
