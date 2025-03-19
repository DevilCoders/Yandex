#pragma once

#include "offroad_block_string_wad_reader.h"
#include "offroad_block_string_wad_sampler.h"
#include "offroad_block_string_wad_searcher.h"
#include "offroad_block_string_wad_writer.h"

#include <kernel/doom/wad/wad_index_type.h>

namespace NDoom {

/*
    Implements disk IO block indices with in-memory subindex.
    blockSize shows how often we would flush the subindex.
*/

template <
    EWadIndexType indexType,
    size_t blockSize = 64>
struct TOffroadBlockStringWadIo {
    static_assert(blockSize != 0, "Block size must be positive");

    using TReader = TOffroadBlockStringWadReader<indexType, blockSize>;
    using TWriter = TOffroadBlockStringWadWriter<indexType, blockSize>;
    using TSearcher = TOffroadBlockStringWadSearcher<indexType, blockSize>;
    using TSampler = TOffroadBlockStringWadSampler<blockSize>;

    static constexpr EWadIndexType IndexType = indexType;

    enum {
        HasSampler = true,
    };
};

}
