#pragma once

#include <kernel/doom/standard_models/standard_models.h>
#include <kernel/doom/wad/wad_index_type.h>

#include "offroad_inv_wad_sampler.h"
#include "offroad_inv_wad_writer.h"
#include "offroad_inv_wad_reader.h"
#include "offroad_inv_wad_searcher.h"

namespace NDoom {


template <EWadIndexType indexType, class Data, class Vectorizer, class Subtractor, class PrefixVectorizer, EStandardIoModel defaultModel>
struct TOffroadInvWadIo {
    using TSampler = TOffroadInvWadSampler<Data, Vectorizer, Subtractor, PrefixVectorizer>;
    using TWriter = TOffroadInvWadWriter<indexType, Data, Vectorizer, Subtractor, PrefixVectorizer>;
    using TReader = TOffroadInvWadReader<indexType, Data, Vectorizer, Subtractor, PrefixVectorizer>;
    using TSearcher = TOffroadInvWadSearcher<indexType, Data, Vectorizer, Subtractor, PrefixVectorizer>;

    using TModel = typename TWriter::TModel;

    constexpr static EWadIndexType IndexType = indexType;
    constexpr static EStandardIoModel DefaultModel = defaultModel;
};


} // namespace NDoom
