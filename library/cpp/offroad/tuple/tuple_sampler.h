#pragma once

#include <library/cpp/offroad/codec/interleaved_sampler.h>
#include <library/cpp/offroad/codec/sampler_64.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "tuple_output_buffer.h"

namespace NOffroad {
    template <class Data, class Vectorizer, class Subtractor, class BaseSampler = TSampler64, EBufferType bufferType = AutoEofBuffer>
    class TTupleSampler {
        using TSampler = TInterleavedSampler<Vectorizer::TupleSize, BaseSampler>;

    public:
        using THit = Data;
        using TModel = typename TSampler::TModel;
        using TTable = void;
        using TPosition = TDataOffset;

        enum {
            TupleSize = Vectorizer::TupleSize,
            BlockSize = TSampler::BlockSize,
            Stages = TSampler::Stages,
        };

        TTupleSampler() {
        }

        void Reset() {
            Sampler_.Reset();
            Buffer_.Reset();
        }

        void WriteHit(const THit& data) {
            Buffer_.WriteHit(data);

            if (Buffer_.IsDone())
                FlushBuffer();
        }

        void WriteSeekPoint() {
            Buffer_.WriteSeekPoint();
        }

        void FinishBlock() {
            Buffer_.Finish();
            FlushBuffer();
            Buffer_.Reset();
        }

        TModel Finish() {
            if (IsFinished())
                return TModel();

            Buffer_.Finish();
            FlushBuffer();
            return Sampler_.Finish();
        }

        bool IsFinished() const {
            return Sampler_.IsFinished();
        }

        TDataOffset Position() {
            return TDataOffset();
        }

    private:
        void FlushBuffer() {
            Buffer_.Flush(&Sampler_);
        }

    private:
        TSampler Sampler_;
        TTupleOutputBuffer<THit, Vectorizer, Subtractor, BlockSize, bufferType> Buffer_;
    };

}
