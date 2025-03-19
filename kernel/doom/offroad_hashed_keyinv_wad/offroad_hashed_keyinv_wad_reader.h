#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_reader.h>
#include <kernel/doom/offroad_block_wad/offroad_block_wad_reader.h>
#include <kernel/doom/wad/wad.h>
#include <util/generic/string.h>

#include <utility>

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
class TOffroadHashedKeyInvWadReader {
    using TBlockReader = TOffroadBlockWadReader<indexType, Hash, HashVectorizer, HashSubtractor, codec, blockSize>;
    using THitReader = TOffroadDocWadReader<indexType, Hit, Vectorizer, Subtractor, NOffroad::TNullVectorizer, codec>;
public:
    using THash = Hash;
    using TBlockModel = typename TBlockReader::TModel;

    using THit = Hit;
    using THitModel = typename THitReader::TModel;

    TOffroadHashedKeyInvWadReader() = default;

    template <typename... Args>
    TOffroadHashedKeyInvWadReader(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TString& blockPath, const TString& hitPath) {
        BlockReader_.Reset(blockPath);
        HitReader_.Reset(hitPath);
    }

    void Reset(const IWad* blockWad, const IWad* hitWad) {
        BlockReader_.Reset(blockWad);
        HitReader_.Reset(hitWad);
    }

    bool ReadHit(THit* hit) {
        return HitReader_.ReadHit(hit);
    }

    bool ReadHash(THash* hash) {
        ui32 docId = 0;
        if (!HitReader_.ReadDoc(&docId)) {
            return false;
        }
        return BlockReader_.ReadHash(hash);
    }

private:
    TBlockReader BlockReader_;
    THitReader HitReader_;
};


} // namespace NDoom
