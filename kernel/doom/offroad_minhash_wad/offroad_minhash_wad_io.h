#pragma once

#include "offroad_minhash_wad_reader.h"
#include "offroad_minhash_wad_searcher.h"
#include "offroad_minhash_wad_writer.h"

namespace NDoom {

template <typename T, typename Key, typename Data>
concept MinHashKeyValueIo = requires {
    MinHashKeyValueWriter<typename T::TWriter, Key, Data>;
    MinHashKeyValueSearcher<typename T::TSearcher, Key, Data>;
    MinHashKeyValueReader<typename T::TReader, Key, Data>;
};

template <EWadIndexType indexType, typename Key, typename Data, typename KeySerializer, MinHashKeyValueIo<Key, Data> KeyIo>
struct TOffroadMinHashWadIo {
    using TWriter = TOffroadMinHashWadWriter<indexType, Key, Data, KeySerializer, typename KeyIo::TWriter>;
    using TSearcher = TOffroadMinHashWadSearcher<indexType, Key, Data, KeySerializer, typename KeyIo::TSearcher>;
    using TReader = typename KeyIo::TReader;

    inline static constexpr EWadIndexType IndexType = indexType;
};

} // namespace NDoom
