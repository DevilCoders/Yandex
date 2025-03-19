#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_sampler.h>

#include <utility>

namespace NDoom {

template <
    class Hash,
    class HashVectorizer,
    class HashSubtractor,
    size_t blockSize>
class TOffroadBlockWadSampler {
    using TBlockSampler = TOffroadDocWadSampler<Hash, HashVectorizer, HashSubtractor>;

public:
    using THash = Hash;
    using TModel = typename TBlockSampler::TModel;

    enum {
        Stages = TBlockSampler::Stages,
    };

    TOffroadBlockWadSampler() = default;

    void Reset() {
        HashId_ = 0;
        BlockId_ = 0;
        Flushed_ = false;
        BlockSampler_.Reset();
    }

    void WriteBlock(const THash& hash, ui32* hashId) {
        BlockSampler_.WriteHit(hash);
        Flushed_ = false;
        if (HashId_ % blockSize == blockSize - 1) {
            Flushed_ = true;
            // we don't want too many blocks
            Y_ENSURE(BlockId_ != Max<ui32>());
            BlockSampler_.WriteDoc(BlockId_++);
        }
        *hashId = HashId_;
        Y_ENSURE(HashId_ != Max<ui32>());
        ++HashId_;
    }

    TModel Finish() {
        if (!Flushed_ && !BlockSampler_.IsFinished()) {
            BlockSampler_.WriteDoc(BlockId_);
        }
        return !BlockSampler_.IsFinished() ? BlockSampler_.Finish() : TModel();
    }

    bool IsFinished() const {
        return BlockSampler_.IsFinished();
    }

private:
    ui32 HashId_ = 0;
    ui32 BlockId_ = 0;
    TBlockSampler BlockSampler_;
    bool Flushed_ = false;
};

}
