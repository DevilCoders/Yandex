#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_codec.h>
#include <kernel/doom/offroad_block_wad/offroad_block_wad_reader.h>
#include <kernel/doom/offroad_block_wad/offroad_block_wad_sampler.h>
#include <kernel/doom/offroad_block_wad/offroad_block_wad_searcher.h>
#include <kernel/doom/offroad_block_wad/offroad_block_wad_writer.h>
#include <kernel/doom/standard_models/standard_models.h>
#include <kernel/doom/wad/wad_index_type.h>

namespace NDoom {

/*
    Implements disk IO block indices  with in-memory subindex.
    blockSize shows how often we would flush the subindex.
*/

template <
    EWadIndexType indexType,
    class Hash,
    class HashVectorizer,
    class HashSubtractor,
    EStandardIoModel defaultHashModel,
    EOffroadDocCodec codec = BitDocCodec,
    size_t blockSize = 64>
struct TOffroadBlockWadIo {
    static_assert(blockSize != 0, "Block size must be positive");

    using TReader = TOffroadBlockWadReader<indexType, Hash, HashVectorizer, HashSubtractor, codec, blockSize>;
    using TWriter = TOffroadBlockWadWriter<indexType, Hash, HashVectorizer, HashSubtractor, codec, blockSize>;
    using TSearcher = TOffroadBlockWadSearcher<indexType, Hash, HashVectorizer, HashSubtractor, codec, blockSize>;
    using TSampler = TOffroadBlockWadSampler<Hash, HashVectorizer, HashSubtractor, blockSize>;

    static constexpr EWadIndexType IndexType = indexType;
    static constexpr EStandardIoModel DefaultHashModel = defaultHashModel;

    enum {
        HasSampler = true,
    };
};

}
