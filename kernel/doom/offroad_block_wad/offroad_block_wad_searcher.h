#pragma once

#include <kernel/doom/wad/mega_wad.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_iterator.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_searcher.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/flat/flat_ui32_searcher.h>
#include <library/cpp/offroad/flat/flat_ui64_searcher.h>

#include <util/memory/blob.h>
#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>
#include <util/system/compiler.h>

#include <cstddef>

#include <type_traits>

namespace NDoom {

template <
    EWadIndexType indexType,
    class Hash,
    class HashVectorizer,
    class HashSubtractor,
    EOffroadDocCodec codec,
    size_t blockSize>
class TOffroadBlockWadSearcher {
    using TSubSearcher = NOffroad::TFlatSearcher<std::nullptr_t, Hash, NOffroad::TNullVectorizer, HashVectorizer>;
    using TBlockSearcher = TOffroadDocWadSearcher<indexType, Hash, HashVectorizer, HashSubtractor, NOffroad::TNullVectorizer, codec>;
public:
    using THash = Hash;
    using THashIterator = TOffroadDocWadIterator<Hash, HashVectorizer, HashSubtractor, NOffroad::TNullVectorizer, codec>;

    TOffroadBlockWadSearcher() = default;

    TOffroadBlockWadSearcher(const IWad* subWad, const IWad* blockWad) {
        Reset(subWad, blockWad);
    }

    void Reset(const IWad* subWad, const IWad* blockWad) {
        SubSearcher_.Reset(subWad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSub)));
        BlockSearcher_.Reset(blockWad);

        if (blockWad->HasGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize))) {
            TBlob sizeBlob = blockWad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize));
            Y_ENSURE(sizeBlob.Size() == sizeof(ui32));
            Size_ = ReadUnaligned<ui32>(sizeBlob.Data());
        }
    }

    bool Find(const THash& hash, ui32 *hashId) const {
        return FindHashId(hash, hashId);
    }

    TVector<bool> Find(const TArrayRef<const THash>& hashes, TArrayRef<ui32> hashIds) const {
        return FindInternal(hashes, hashIds);
    }

    inline ui32 Size() const {
        return Size_;
    }

private:

    inline ui32 FindBlock(const THash& hash) const {
        auto cmp = [&](size_t l, const THash& r) {
            return SubSearcher_.ReadData(l) < r;
        };

        auto range = xrange<size_t>(0, SubSearcher_.Size());

        return *::LowerBound(range.begin(), range.end(), hash, cmp);
    }

    inline bool FindHashId(const THash& hash, ui32* hashId) const {
        ui32 blockId = FindBlock(hash);
        THashIterator iter;

        if (!BlockSearcher_.Find(blockId, &iter)) {
            return false;
        }

        THash firstHash = THash();
        size_t index = 0;
        if (!iter.ReadHit(&firstHash)) {
            return false;
        }

        while (firstHash < hash && iter.ReadHit(&firstHash)) {
            ++index;
        }
        if (hash != firstHash) {
            return false;
        }

        size_t foundHashId = blockSize * static_cast<size_t>(blockId) + index;

        // unlikely scenario, but can happen
        if (Y_UNLIKELY(foundHashId > static_cast<size_t>(Max<ui32>()))) {
            return false;
        }

        *hashId = static_cast<ui32>(foundHashId);

        return true;
    }

    /*
        Hashes must be sorted
    */
    TVector<bool> FindInternal(const TArrayRef<const THash>& hashes, TArrayRef<ui32> hashIds) const {
        const size_t hashesSize = hashes.size();
        TVector<ui32> blocks(hashesSize);
        TVector<bool> found(hashesSize);
        for (size_t i = 0; i < hashesSize; ++i) {
            blocks[i] = FindBlock(hashes[i]);
        }
        Y_ASSERT(IsSorted(blocks.begin(), blocks.end()));
        THashIterator iter;
        BlockSearcher_.PrefetchDocs(blocks, &iter);
        ui32 block = 0;
        size_t index = 0;
        bool blockIsEmpty = true;
        THash hash = THash();
        for (size_t i = 0; i < hashesSize; ++i) {
            if (i == 0 || block != blocks[i]) {
                block = blocks[i];
                index = 0;
                blockIsEmpty = !BlockSearcher_.Find(blocks[i], &iter);
                if (!blockIsEmpty) {
                    blockIsEmpty = !iter.ReadHit(&hash);
                }
            }
            while (!blockIsEmpty && hash < hashes[i]) {
                if (iter.ReadHit(&hash)) {
                    ++index;
                } else {
                    blockIsEmpty = true;
                }
            }
            if (blockIsEmpty || hashes[i] != hash) {
                continue;
            }
            size_t hashId = blockSize * static_cast<size_t>(block) + index;
            // unlikely scenario, but can happen
            if (Y_UNLIKELY(hashId > static_cast<size_t>(Max<ui32>()))) {
                continue;
            }
            hashIds[i] = static_cast<ui32>(hashId);
            found[i] = true;
        }
        return found;
    }

    TSubSearcher SubSearcher_;
    TBlockSearcher BlockSearcher_;
    ui32 Size_ = 0;
};

} // namespace NDoom
