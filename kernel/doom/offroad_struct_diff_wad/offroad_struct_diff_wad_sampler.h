#pragma once

#include <kernel/doom/wad/wad.h>
#include <library/cpp/offroad/codec/interleaved_sampler.h>
#include <library/cpp/offroad/codec/sampler_64.h>

#include "struct_diff_common.h"
#include "struct_diff_output_buffer.h"


namespace NDoom {


template <class Key, class KeyVectorizer, class KeySubtractor, class Data>
class TOffroadStructDiffWadSampler {
    using TBuffer = TStructDiffOutputBuffer<NOffroad::TSampler64::BlockSize, Key, KeyVectorizer, KeySubtractor, Data>;

public:
    enum {
        BlockSize = TBuffer::BlockSize,
        Stages = NOffroad::TSampler64::Stages
    };

    using TKey = Key;
    using TData = Data;
    using TModel = TStructDiffModel<KeyVectorizer::TupleSize, sizeof(TData), NOffroad::TSampler64::TModel>;

    void Reset() {
        Sampler_.Reset();

        Buffer_.Reset();
    }

    void Write(const TKey& key, const TData* data) {
        Buffer_.Write(key, data);

        if (Buffer_.IsDone()) {
            Buffer_.Flush(&Sampler_);
        }
    }

    TModel Finish() {
        Buffer_.Flush(&Sampler_);
        return Sampler_.Finish();
    }

private:
    NOffroad::TInterleavedSampler<TModel::TupleSize, NOffroad::TSampler64> Sampler_;
    TBuffer Buffer_;
};


} // namespace NDoom
