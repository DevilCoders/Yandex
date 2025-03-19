#pragma once

#include <library/cpp/offroad/keyinv/null_keyinv_sampler.h>
#include <library/cpp/offroad/keyinv/null_model.h>

#include <kernel/doom/adaptors/encoding_index_writer.h>
#include <kernel/doom/adaptors/key_transforming_index_writer.h>
#include <kernel/doom/adaptors/multi_key_index_writer.h>
#include <kernel/doom/key/old_key_decoder.h>
#include <kernel/doom/key/key_decoding_transformation.h>
#include <kernel/doom/key/key_encoder.h>
#include <kernel/doom/standard_models/standard_models.h>
#include <kernel/doom/wad/wad_index_type.h>

#include "offroad_wad_sampler.h"
#include "offroad_wad_writer.h"
#include "offroad_wad_searcher.h"

namespace NDoom {

template<class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer, EStandardIoModel DefaultKeyIoModel = NoStandardIoModel, EStandardIoModel DefaultHitIoModel = NoStandardIoModel, bool UseDecodedKey = false, NDoom::EWadIndexType indexType = NDoom::EWadIndexType::FactorAnnIndexType>
struct TOffroadWadIo {
    constexpr static EWadIndexType IndexType = indexType;

    using THitSampler = NOffroad::TNullKeyInvSampler;
    using TKeySampler = NOffroad::TNullKeyInvSampler;
    using TUniSampler = std::conditional_t<
        UseDecodedKey,
        TKeyTransformingIndexWriter<TMultiKeyIndexWriter<TEncodingIndexWriter<TOffroadWadSampler<Hit, Vectorizer, Subtractor>, TKeyEncoder>>, TKeyDecodingTransformation<TOldKeyDecoder>, TString, TStringBuf>,
        TOffroadWadSampler<Hit, Vectorizer, Subtractor>
    >;
    using TWriter = std::conditional_t<
        UseDecodedKey,
        TKeyTransformingIndexWriter<TMultiKeyIndexWriter<TEncodingIndexWriter<TOffroadWadWriter<IndexType, Hit, Vectorizer, Subtractor, PrefixVectorizer, NOffroad::TKeyFullPrefixGetter>, TKeyEncoder>>, TKeyDecodingTransformation<TOldKeyDecoder>, TString, TStringBuf>,
        TOffroadWadWriter<IndexType, Hit, Vectorizer, Subtractor, PrefixVectorizer, NOffroad::TKeyFullPrefixGetter>
    >;
    using TSearcher = TOffroadWadSearcher<IndexType, Hit, Vectorizer, Subtractor, PrefixVectorizer>;
    using TKeySearcher = TOffroadWadKeySearcher<IndexType, Hit, Vectorizer, Subtractor, PrefixVectorizer>;
    using THitSearcher = TOffroadWadHitSearcher<IndexType, Hit, Vectorizer, Subtractor, PrefixVectorizer>;
    using ISearcher = typename TSearcher::TBase;
    using IKeySearcher = typename TKeySearcher::TBase;
    using IHitSearcher = typename THitSearcher::TBase;

    constexpr static EStandardIoModel DefaultHitModel = DefaultHitIoModel;
    constexpr static EStandardIoModel DefaultKeyModel = DefaultKeyIoModel;

    enum {
        HasHitSampler = false,
        HasKeySampler = false,
        HasUniSampler = true,
    };

    using THitModel = typename TWriter::THitModel;
    using TKeyModel = typename TWriter::TKeyModel;
};

} // namespace NDoom
