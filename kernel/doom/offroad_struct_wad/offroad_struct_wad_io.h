#pragma once

#include "offroad_struct_wad_sampler.h"
#include "offroad_struct_wad_writer.h"
#include "offroad_struct_wad_reader.h"
#include "offroad_struct_wad_searcher.h"

#include <kernel/doom/standard_models/standard_models.h>


namespace NDoom {


template <EWadIndexType indexType, class Data, class Serializer, EStructType structType, ECompressionType compressionType = OffroadCompressionType, EStandardIoModel defaultModel = NoStandardIoModel>
struct TOffroadStructWadIo {
    using TSampler = TOffroadStructWadSampler<Data, Serializer, structType, compressionType>;
    using TWriter = TOffroadStructWadWriter<indexType, Data, Serializer, structType, compressionType>;
    using TReader = TOffroadStructWadReader<indexType, Data, Serializer, structType, compressionType>;
    using TSearcher = TOffroadStructWadSearcher<indexType, Data, Serializer, structType, compressionType>;
    using TSerializer = Serializer;
    using THit = Data;

    using TModel = typename TWriter::TModel;

    constexpr static EWadIndexType IndexType = indexType;
    constexpr static EStructType StructType = structType;
    constexpr static ECompressionType CompressionType = compressionType;

    enum {
        HasSampler = (compressionType != RawCompressionType),
    };

    constexpr static EStandardIoModel DefaultModel = defaultModel;
};


} // namespace NDoom
