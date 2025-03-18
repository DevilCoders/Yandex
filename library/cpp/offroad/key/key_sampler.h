#pragma once

#include <library/cpp/offroad/codec/interleaved_sampler.h>
#include <library/cpp/offroad/codec/sampler_64.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "key_output_buffer.h"
#include "key_model.h"

namespace NOffroad {
    template <class KeyData, class Vectorizer, class Subtractor, EKeySubtractor keySubtractor = DeltaKeySubtractor>
    class TKeySampler {
        using TOutputBuffer = TKeyOutputBuffer<KeyData, Vectorizer, Subtractor, 64, keySubtractor>;
        using TSampler = TInterleavedSampler<TOutputBuffer::TupleSize, TSampler64>;

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TKeyData = KeyData;
        using TTable = void;
        using TModel = TKeyModel<typename TSampler::TModel>;

        enum {
            Stages = TSampler::Stages,
        };

        TKeySampler() {
        }

        void WriteKey(const TKeyRef& key, const TKeyData& data) {
            Buffer_.Write(key, data);

            if (Buffer_.IsDone())
                Buffer_.Flush(&Sampler_);
        }

        void Reset() {
            Sampler_.Reset();
            Buffer_.Reset();
        }

        TModel Finish() {
            TModel result;
            if (IsFinished())
                return result;

            if (Buffer_.BlockPosition() != 0) {
                Buffer_.Finish();
                Buffer_.Flush(&Sampler_);
            }

            result.Base_ = Sampler_.Finish();
            return result;
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
