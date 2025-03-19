#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_reader.h>
#include <kernel/doom/wad/wad.h>
#include <util/generic/string.h>

#include <utility>

namespace NDoom {

template <
    EWadIndexType indexType,
    class Hash,
    class HashVectorizer,
    class HashSubtractor,
    EOffroadDocCodec codec,
    size_t blockSize>
class TOffroadBlockWadReader {
    using TBlockReader = TOffroadDocWadReader<indexType, Hash, HashVectorizer, HashSubtractor, NOffroad::TNullVectorizer, codec>;
public:
    using THash = Hash;
    using TModel = typename TBlockReader::TModel;

    TOffroadBlockWadReader() = default;

    template <typename... Args>
    TOffroadBlockWadReader(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TString& blockPath) {
        BlockReader_.Reset(blockPath);
    }

    void Reset(const IWad* blockWad) {
        BlockReader_.Reset(blockWad);
    }

    bool ReadHash(THash* hash) {
        ui32 docId = 0;
        if (BlockReader_.ReadHit(hash)) {
            return true;
        }
        if (!BlockReader_.ReadDoc(&docId)) {
            return false;
        }
        Y_ENSURE(BlockReader_.ReadHit(hash));
        return true;
    }

private:
    TBlockReader BlockReader_;
};

} // namespace NDoom
