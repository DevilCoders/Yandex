#pragma once

#include <util/system/yassert.h>
#include <util/generic/ptr.h>

#include <library/cpp/offroad/codec/interleaved_sampler.h>
#include <library/cpp/offroad/codec/sampler_64.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "trie_output_buffer.h"

namespace NOffroad {
    class TTrieSampler {
        using TOutputBuffer = TTrieOutputBuffer<64>;
        using TSampler = TInterleavedSampler<TOutputBuffer::TupleSize, TSampler64>;

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TData = TStringBuf;
        using TTable = void;
        using TModel = typename TSampler::TModel;

        enum {
            BlockSize = 64,
        };

        TTrieSampler() {
        }

        void Reset() {
            Sampler_.Reset();
            Buffer_.Reset();
        }

        void Write(const TKeyRef& key, const TData& data) {
            Buffer_.Write(key, data);

            if (Buffer_.IsDone())
                Buffer_.Flush(&Sampler_);
        }

        TModel Finish() {
            if (IsFinished())
                return TModel();

            if (Buffer_.BlockPosition() != 0) {
                Buffer_.Finish();
                Buffer_.Flush(&Sampler_);
            }

            return Sampler_.Finish();
        }

        bool IsFinished() const {
            return Sampler_.IsFinished();
        }

        TDataOffset Position() const {
            return TDataOffset(0, Buffer_.BlockPosition());
        }

    private:
        TSampler Sampler_;
        TOutputBuffer Buffer_;
    };

}
