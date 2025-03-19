#pragma once

#include "compression_type.h"
#include "struct_type.h"

#include <library/cpp/offroad/byte_stream/byte_output_stream.h>
#include <library/cpp/offroad/byte_stream/raw_output_stream.h>


namespace NDoom {


template <class Data, class Serializer, EStructType structType, ECompressionType compressionType>
class TOffroadStructWadSampler {
    static constexpr bool IsAutoEofStruct = (structType == AutoEofStructType);

    using TSampler = std::conditional_t<
        compressionType == OffroadCompressionType,
        std::conditional_t<
            IsAutoEofStruct,
            NOffroad::TByteSampleStreamEof,
            NOffroad::TByteSampleStream
        >,
        NOffroad::TRawSampleStream
    >;
public:
    using THit = Data;
    using TModel = typename TSampler::TModel;

    TOffroadStructWadSampler()
        : Sampler_(&Model_)
    {

    }

    void Reset() {
        Sampler_.Reset(&Model_);
    }

    void WriteDoc(ui32) {
    }

    void WriteHit(const THit& data) {
        Serializer::Serialize(data, &Sampler_);
        Sampler_.Flush();
    }

    void Write(ui32 /*docId*/, const THit& data) {
        WriteHit(data);
    }

    bool IsFinished() const {
        return Sampler_.IsFinished();
    }

    TModel Finish() {
        if (IsFinished()) {
            return Model_;
        }
        Sampler_.Finish();
        return Model_;
    }

private:
    TSampler Sampler_;
    TModel Model_;
};


} // namespace NDoom
