#pragma once

#include <kernel/doom/offroad_block_wad/offroad_block_wad_searcher.h>

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
    class Hit,
    class Vectorizer,
    class Subtractor,
    EOffroadDocCodec codec,
    size_t blockSize>
class TOffroadHashedKeyInvWadSearcher {
    using TBlockSearcher = TOffroadBlockWadSearcher<indexType, Hash, HashVectorizer, HashSubtractor, codec, blockSize>;
    using THitSearcher = TOffroadDocWadSearcher<indexType, Hit, Vectorizer, Subtractor, NOffroad::TNullVectorizer, codec>;
public:
    using THash = Hash;
    using THit = Hit;
    using THitIterator = TOffroadDocWadIterator<Hit, Vectorizer, Subtractor, NOffroad::TNullVectorizer, codec>;

    TOffroadHashedKeyInvWadSearcher() = default;

    TOffroadHashedKeyInvWadSearcher(const IWad* subWad, const IWad* blockWad, const IWad* hashWad) {
        Reset(subWad, blockWad, hashWad);
    }

    void Reset(const IWad* subWad, const IWad* blockWad, const IWad* hashWad) {
        BlockSearcher_.Reset(subWad, blockWad);
        HitSercher_.Reset(hashWad);
    }

    bool Find(const THash& hash, THitIterator* iterator) const {
        return FindInternal(hash, iterator);
    }

    TVector<bool> FindMany(const TVector<THash>& hashes, TVector<THitIterator>* iterators) const {
        TVector<ui32> hashIds(hashes.size(), 0);
        TVector<bool> found = BlockSearcher_.Find(hashes, hashIds);
        size_t filteredHashIdIdx = 0;
        for (size_t i = 0; i < hashIds.size(); ++i) {
            if (found[i]) {
                hashIds[filteredHashIdIdx++] = hashIds[i];
            }
        }
        hashIds.resize(filteredHashIdIdx);
        THitIterator iterator;
        HitSercher_.PrefetchDocs(hashIds, &iterator);
        iterators->resize(hashes.size());
        Y_ASSERT(iterator.PrefetchedDocIds().size() == hashIds.size());
        size_t foundIdx = 0;
        for (size_t i = 0; i < iterator.PrefetchedDocIds().size(); ++i) {
            THitIterator iter;
            while (foundIdx < found.size() && !found[foundIdx]) {
                ++foundIdx;
            }
            if (HitSercher_.Find(iterator.PrefetchedDocIds()[i], &iterator)) {
                iter.Blob().Swap(iterator.Blob());
                iter.Reader_.Swap(iterator.Reader_);
                (*iterators)[foundIdx].Swap(iter);
            } else {
                found[foundIdx] = false;
            }
            ++foundIdx;
        }
        return found;
    }

    inline ui32 Size() const {
        return BlockSearcher_.Size();
    }

private:

    bool FindInternal(const THash& hash, THitIterator* iterator) const {
        ui32 hashId = 0;
        if (BlockSearcher_.Find(hash, &hashId)) {
            if (HitSercher_.Find(hashId, iterator)) {
                return true;
            }
        }
        return false;
    }

    TBlockSearcher BlockSearcher_;
    THitSearcher HitSercher_;
};

} // namespace NDoom
