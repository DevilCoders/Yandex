#pragma once

#include <kernel/reqbundle/block.h>

#include <search/base/blob_cache/lru_cache.h>

#include <util/memory/blob.h>

namespace NReqBundleIterator {
    enum {
        KeyPrefixSize = 256
    };

    Y_FORCE_INLINE size_t KeySize(const NReqBundle::TBinaryBlock& block) {
        return Min<size_t>(KeyPrefixSize, block.GetData().Size());
    }

    struct TBinaryBlockHash {
        Y_FORCE_INLINE size_t operator()(const NReqBundle::TBinaryBlock& binary) const {
            return binary.GetHash();
        }
    };

    struct TBinaryBlockEq {
        Y_FORCE_INLINE bool operator()(
            const NReqBundle::TBinaryBlock& binaryX,
            const NReqBundle::TBinaryBlock& binaryY) const
        {
            const size_t sizeX = KeySize(binaryX);
            return binaryX.GetHash() == binaryY.GetHash()
                && sizeX == KeySize(binaryY)
                && 0 == memcmp(binaryX.GetData().AsCharPtr(), binaryY.GetData().AsCharPtr(), sizeX);
        }
    };

    struct TBinaryBlockCopier {
        Y_FORCE_INLINE size_t GetSize(const NReqBundle::TBinaryBlock& key) const {
            return KeySize(key);
        }

        Y_FORCE_INLINE NReqBundle::TBinaryBlock Realloc(const NReqBundle::TBinaryBlock& key, char* buf) const {
            const size_t keySize = KeySize(key);
            memcpy(buf, key.GetData().AsCharPtr(), keySize);
            NReqBundle::TBinaryBlock res;
            NReqBundle::NDetail::BackdoorAccess(res).Data = TBlob::NoCopy(buf, keySize);
            NReqBundle::NDetail::BackdoorAccess(res).Hash = key.GetHash();
            return res;
        }
    };

    struct TDefaultHasherTraits {
        enum {
            NumPages = 160000,
            PageSize = 256,
            NumPagesPerItem = 32
        };
    };

    constexpr NBaseSearch::TLruBlobCacheOptions DefaultCacheOptions() {
        return NBaseSearch::TLruBlobCacheOptions {
            .PagesCount = TDefaultHasherTraits::NumPages,
            .PageSize = TDefaultHasherTraits::PageSize,
            .MaxPagesPerItem = TDefaultHasherTraits::NumPagesPerItem
        };
    };

    constexpr NBaseSearch::TLruBlobCacheOptions ShrinkCacheOptions(ui32 n) {
        return NBaseSearch::TLruBlobCacheOptions {
            .PagesCount = TDefaultHasherTraits::NumPages / n,
            .PageSize = TDefaultHasherTraits::PageSize,
            .MaxPagesPerItem = TDefaultHasherTraits::NumPagesPerItem / n
        };
    };

    constexpr NBaseSearch::TLruBlobCacheOptions GrowCacheOptions(ui32 n) {
        return NBaseSearch::TLruBlobCacheOptions {
            .PagesCount = TDefaultHasherTraits::NumPages * n,
            .PageSize = TDefaultHasherTraits::PageSize,
            .MaxPagesPerItem = TDefaultHasherTraits::NumPagesPerItem * n
        };
    };

    using TRBHasher = NBaseSearch::TLruBlobCache<NReqBundle::TBinaryBlock,
        TBinaryBlockHash,
        TBinaryBlockEq>;
}
