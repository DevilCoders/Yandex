#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_sampler.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_writer.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_reader.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_searcher.h>

#include <kernel/doom/wad/wad_index_type.h>
#include <kernel/doom/standard_models/standard_models.h>

namespace NDoom {

template <EWadIndexType indexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer, EStandardIoModel defaultHitModel, EOffroadDocCodec codec = AdaptiveDocCodec>
struct TOffroadDocWadIo {
    using TSampler = TOffroadDocWadSampler<Hit, Vectorizer, Subtractor>;
    using TWriter = TOffroadDocWadWriter<indexType, Hit, Vectorizer, Subtractor, PrefixVectorizer, codec>;
    using TReader = TOffroadDocWadReader<indexType, Hit, Vectorizer, Subtractor, PrefixVectorizer, codec>;
    using TSearcher = TOffroadDocWadSearcher<indexType, Hit, Vectorizer, Subtractor, PrefixVectorizer, codec>;

    using TPrefixVectorizer = PrefixVectorizer;

    using TModel = typename TWriter::TModel;

    constexpr static EWadIndexType IndexType = indexType;
    constexpr static EStandardIoModel DefaultModel = defaultHitModel;

    enum {
        HasSampler = true,
    };
};

} // namespace NDoom
