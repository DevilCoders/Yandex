#pragma once

#include "blob_storage.h"
#include "item_chunk_mapper.h"

#include <search/base/blob_cache/lru_cache.h>

namespace NDoom::NItemStorage {

using TBlobStorageCacheOptions = NBaseSearch::TLruBlobCacheOptions;
using TBlobStorageCacheStats = NBaseSearch::TLruBlobCacheStats;

class TBlobStorageCache : public TAtomicRefCount<TBlobStorageCache> {
public:
    struct TCacheKey {
        TItemChunk Chunk;
        ui128 ChunkRevision = 0;

        auto operator<=>(const TCacheKey& rhs) const = default;
    };

private:
    struct TCacheKeyHasher {
        size_t operator()(const TCacheKey& key) const {
            auto hash = []<typename T>(const T& value) {
                return THash<T>{}(value);
            };

            return CombineHashes(CombineHashes(hash(key.Chunk.Chunk), hash(key.Chunk.Index)), hash(key.ChunkRevision));
        }
    };

    struct TLoadHelper {
        using TResult = TMaybe<TItemBlob>;
        static void Load(const TCacheKey& key, TBlob blob, TMaybe<TItemBlob>& result) {
            result.ConstructInPlace();
            result->Blob = blob;
            result->Chunk = key.Chunk.Chunk;
        }

        static bool Expired(const TCacheKey&) {
            return false;
        }
    };

    struct TStoreHelper {
        using TResult = TMaybe<TItemBlob>;
        static const TBlob& GetBlob(const TMaybe<TItemBlob>& result) {
            return result->Blob.GetRef();
        }
        static bool Check(const TMaybe<TItemBlob>& result, size_t maxSize) {
            return result && result->Blob->Size() <= maxSize;
        }
    };
public:
    explicit TBlobStorageCache(const TBlobStorageCacheOptions& options)
        : Cache_(options)
    {
    }

    TBlobStorageCacheStats Load(TConstArrayRef<TCacheKey> keys, TArrayRef<TMaybe<TItemBlob>> results, TArrayRef<bool> loaded) {
        return Cache_.Load<TLoadHelper>(keys, results, loaded);
    }

    TBlobStorageCacheStats Store(TConstArrayRef<TCacheKey> keys, TConstArrayRef<TMaybe<TItemBlob>> results, TConstArrayRef<bool> loaded) {
        return Cache_.Store<TStoreHelper>(keys, results, loaded);
    }

    NBaseSearch::TLruBlobCache<TCacheKey, TCacheKeyHasher> Cache_;
};

using TBlobStorageCachePtr = TIntrusivePtr<TBlobStorageCache>;

} // namespace NDoom::NItemStorage
