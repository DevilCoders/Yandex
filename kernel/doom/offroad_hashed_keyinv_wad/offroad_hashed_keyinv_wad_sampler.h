#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_sampler.h>
#include <kernel/doom/offroad_block_wad/offroad_block_wad_sampler.h>

#include <utility>

namespace NDoom {

template <
    class Hash,
    class HashVectorizer,
    class HashSubtractor,
    class Hit,
    class HitVectorizer,
    class HitSubtractor,
    size_t blockSize>
class TOffroadHashedKeyInvWadSampler {
    using TBlockSampler = TOffroadBlockWadSampler<Hash, HashVectorizer, HashSubtractor, blockSize>;
    using THitSampler = TOffroadDocWadSampler<Hit, HitVectorizer, HitSubtractor>;

    static_assert(TBlockSampler::Stages == THitSampler::Stages, "Num stages must be the same");

public:

    using THash = Hash;
    using TBlockModel = typename TBlockSampler::TModel;

    using THit = Hit;
    using THitModel = typename THitSampler::TModel;

    enum {
        Stages = TBlockSampler::Stages,
    };

    TOffroadHashedKeyInvWadSampler() = default;

    void Reset() {
        Flushed_ = false;
        BlockSampler_.Reset();
        HitSampler_.Reset();
    }

    void WriteHit(const THit& hit) {
        HitSampler_.WriteHit(hit);
    }

    void WriteBlock(const THash& hash) {
        ui32 hashId = 0;
        BlockSampler_.WriteBlock(hash, &hashId);
        HitSampler_.WriteDoc(hashId);
    }

    std::pair<TBlockModel, THitModel> Finish() {
        return {
            !BlockSampler_.IsFinished() ? BlockSampler_.Finish() : TBlockModel(),
            !HitSampler_.IsFinished() ? HitSampler_.Finish() : THitModel() };
    }

    bool IsFinished() const {
        return BlockSampler_.IsFinished() && HitSampler_.IsFinished();
    }

private:
    TBlockSampler BlockSampler_;
    THitSampler HitSampler_;
    bool Flushed_ = false;
};

}
