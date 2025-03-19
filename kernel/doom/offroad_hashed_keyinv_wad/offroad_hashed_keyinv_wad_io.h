#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_codec.h>
#include <kernel/doom/offroad_hashed_keyinv_wad/offroad_hashed_keyinv_wad_reader.h>
#include <kernel/doom/offroad_hashed_keyinv_wad/offroad_hashed_keyinv_wad_sampler.h>
#include <kernel/doom/offroad_hashed_keyinv_wad/offroad_hashed_keyinv_wad_searcher.h>
#include <kernel/doom/offroad_hashed_keyinv_wad/offroad_hashed_keyinv_wad_writer.h>
#include <kernel/doom/standard_models/standard_models.h>
#include <kernel/doom/wad/wad_index_type.h>

namespace NDoom {

/*
    Implements disk IO hashed KeyInv with in-memory subindex.
    Hash is (mostly) an integral type, blockSize shows how often we would flush the subindex.
*/

template <
    EWadIndexType indexType,
    class Hash,
    class HashVectorizer,
    class HashSubtractor,
    class Hit,
    class HitVectorizer,
    class HitSubtractor,
    EStandardIoModel defaultHashModel,
    EStandardIoModel defaultHitModel,
    EOffroadDocCodec codec = BitDocCodec,
    size_t blockSize = 64>
struct TOffroadHashedKeyInvWadIo {
    static_assert(blockSize != 0, "Block size must be positive");

    using TReader = TOffroadHashedKeyInvWadReader<indexType, Hash, HashVectorizer, HashSubtractor, Hit, HitVectorizer, HitSubtractor, codec, blockSize>;
    using TWriter = TOffroadHashedKeyInvWadWriter<indexType, Hash, HashVectorizer, HashSubtractor, Hit, HitVectorizer, HitSubtractor, codec, blockSize>;
    using TSearcher = TOffroadHashedKeyInvWadSearcher<indexType, Hash, HashVectorizer, HashSubtractor, Hit, HitVectorizer, HitSubtractor, codec, blockSize>;
    using TIterator = typename TSearcher::THitIterator;
    using TSampler = TOffroadHashedKeyInvWadSampler<Hash, HashVectorizer, HashSubtractor, Hit, HitVectorizer, HitSubtractor, blockSize>;

    static constexpr EWadIndexType IndexType = indexType;
    static constexpr EStandardIoModel DefaultHitModel = defaultHitModel;
    static constexpr EStandardIoModel DefaultHashModel = defaultHashModel;

    enum {
        HasSampler = true,
    };
};

}
