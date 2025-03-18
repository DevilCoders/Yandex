#pragma once

#include "output_stream_base.h"

#include <library/cpp/offroad/codec/bit_sampler_16.h>
#include <library/cpp/offroad/codec/bit_encoder_16.h>

namespace NOffroad {
    template <class Sampler, EBufferType bufferType>
    class TByteSampleStreamBase: public TOutputStreamBase<Sampler, bufferType> {
        using TBase = TOutputStreamBase<Sampler, bufferType>;

    public:
        using TModel = typename Sampler::TModel;

        TByteSampleStreamBase() {
        }

        TByteSampleStreamBase(TModel* model)
            : TBase(model)
        {
        }
    };

    template <class Encoder, EBufferType bufferType>
    class TByteOutputStreamBase: public TOutputStreamBase<Encoder, bufferType> {
        using TBase = TOutputStreamBase<Encoder, bufferType>;

    public:
        using TTable = typename Encoder::TTable;
        using TModel = typename TTable::TModel;

        TByteOutputStreamBase() = default;

        TByteOutputStreamBase(const TTable* table, IOutputStream* output)
            : TBase(table, output)
        {
        }
    };

    template <class Encoder>
    class TEncoderWrapper: public Encoder {
        using TBase = Encoder;

    public:
        using TTable = typename Encoder::TTable;
        using TModel = typename TTable::TModel;

        TEncoderWrapper() {
        }

        TEncoderWrapper(const TTable* table, IOutputStream* stream)
            : TBase(table, &Output_)
        {
            Output_.Reset(stream);
        }

        void Finish() {
            Output_.Finish();
        }

        void Reset(const TTable* table, IOutputStream* stream) {
            Output_.Reset(stream);
            TBase::Reset(table, &Output_);
        }

        ui64 Position() {
            return Output_.Position();
        }

    private:
        TBitOutput Output_;
    };

    template <class Sampler>
    class TSamplerWrapper: public Sampler {
        using TBase = Sampler;

    public:
        using TModel = typename TBase::TModel;

        template <class... Args>
        TSamplerWrapper(TModel* model, Args&&... args)
            : TBase(std::forward<Args>(args)...)
            , Model_(model)
        {
        }

        template <class... Args>
        TSamplerWrapper(Args&&... args)
            : TBase(std::forward<Args>(args)...)
        {
        }

        void Reset(TModel* model) {
            Model_ = model;
            TBase::Reset();
        }

        void Finish() {
            *Model_ = TBase::Finish();
        }

        ui64 Position() {
            return 0;
        }

    private:
        TModel* Model_ = nullptr;
    };

    using TByteSampleStream = TByteSampleStreamBase<TSamplerWrapper<TBitSampler16>, PlainOldBuffer>;
    using TByteOutputStream = TByteOutputStreamBase<TEncoderWrapper<TBitEncoder16>, PlainOldBuffer>;
    using TByteSampleStreamEof = TByteSampleStreamBase<TSamplerWrapper<TBitSampler16>, AutoEofBuffer>;
    using TByteOutputStreamEof = TByteOutputStreamBase<TEncoderWrapper<TBitEncoder16>, AutoEofBuffer>;

} //namespace NOffroad
