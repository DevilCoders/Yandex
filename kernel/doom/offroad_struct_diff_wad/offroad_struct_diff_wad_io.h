#pragma once

#include "offroad_struct_diff_wad_reader.h"
#include "offroad_struct_diff_wad_sampler.h"
#include "offroad_struct_diff_wad_writer.h"
#include "offroad_struct_diff_wad_searcher.h"

#include <kernel/doom/standard_models/standard_models.h>

namespace NDoom {


template <EWadIndexType indexType, class Key, class KeyVectorizer, class KeySubtractor, class KeyPrefixVectorizer, class Data, EStandardIoModel defaultModel>
struct TOffroadStructDiffWadIo {
    static_assert(KeyVectorizer::TupleSize > 0, "Tuple size must be more zero, use ui32 index as key instead or simple StructWad");
    using TSampler = TOffroadStructDiffWadSampler<Key, KeyVectorizer, KeySubtractor, Data>;
    using TWriter = TOffroadStructDiffWadWriter<indexType, Key, KeyVectorizer, KeySubtractor, KeyPrefixVectorizer, Data>;
    using TReader = TOffroadStructDiffWadReader<indexType, Key, KeyVectorizer, KeySubtractor, Data>;
    using TSearcher = TOffroadStructDiffWadSearcher<indexType, Key, KeyVectorizer, KeySubtractor, KeyPrefixVectorizer, Data>;

    using TModel = typename TWriter::TModel;
    using TKey = Key;
    using TData = Data;
    constexpr static EWadIndexType IndexType = indexType;

    enum {
        HasSampler = true,
    };
    static const EStandardIoModel DefaultModel;
};

template <EWadIndexType indexType, class Key, class KeyVectorizer, class KeySubtractor, class KeyPrefixVectorizer, class Data, EStandardIoModel defaultModel>
const EStandardIoModel TOffroadStructDiffWadIo<indexType, Key, KeyVectorizer, KeySubtractor, KeyPrefixVectorizer, Data, defaultModel>::DefaultModel = defaultModel;


} //namespace
