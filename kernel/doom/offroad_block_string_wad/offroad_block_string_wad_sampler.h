#pragma once

#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/key/key_sampler.h>

#include <utility>

namespace NDoom {

template <size_t blockSize>
class TOffroadBlockStringWadSampler {
    using TBlockSampler = NOffroad::TKeySampler<std::nullptr_t, NOffroad::TNullVectorizer, NOffroad::TINSubtractor>;

public:
    using THash = TStringBuf;
    using TModel = typename TBlockSampler::TModel;

    enum {
        Stages = TBlockSampler::Stages,
    };

    TOffroadBlockStringWadSampler() = default;

    void Reset() {
        HashId_ = 0;
        BlockSampler_.Reset();
    }

    void WriteBlock(THash hash) {
        BlockSampler_.WriteKey(hash, nullptr);
        Y_ENSURE(HashId_ != Max<ui32>());
        ++HashId_;
    }

    TModel Finish() {
        return !BlockSampler_.IsFinished() ? BlockSampler_.Finish() : TModel();
    }

    bool IsFinished() const {
        return BlockSampler_.IsFinished();
    }

private:
    ui32 HashId_ = 0;
    TBlockSampler BlockSampler_;
    bool Flushed_ = false;
};

}
