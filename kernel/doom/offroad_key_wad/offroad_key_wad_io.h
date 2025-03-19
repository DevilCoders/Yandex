#pragma once

#include <kernel/doom/standard_models/standard_models.h>
#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>

#include "combiners.h"
#include "offroad_key_wad_reader.h"
#include "offroad_key_wad_writer.h"
#include "offroad_key_wad_sampler.h"
#include "offroad_key_wad_searcher.h"

namespace NDoom {

template <EWadIndexType indexType, class KeyData, class Vectorizer, class Subtractor, class Serializer, class Combiner, EStandardIoModel defaultModel>
struct TOffroadKeyWadIo {
    using TSampler = TOffroadKeyWadSampler<KeyData, Vectorizer, Subtractor>;
    using TWriter = TOffroadKeyWadWriter<indexType, KeyData, Vectorizer, Subtractor, Serializer>;
    using TReader = TOffroadKeyWadReader<indexType, KeyData, Vectorizer, Subtractor, Combiner>;
    using TSearcher = TOffroadKeyWadSearcher<indexType, KeyData, Vectorizer, Subtractor, Serializer, Combiner>;

    using TModel = typename TWriter::TModel;

    constexpr static EWadIndexType IndexType = indexType;
    constexpr static EStandardIoModel DefaultModel = defaultModel;
};

using TOffroadKeyInvKeyWadIo = TOffroadKeyWadIo<KeyInvIndexType, ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor, NOffroad::TNullSerializer, TIdentityCombiner, DefaultAnnSortedMultiKeysKeyIoModel>;
using TOffroadFactorAnnKeyWadIo = TOffroadKeyWadIo<FactorAnnIndexType, ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor, NOffroad::TNullSerializer, TIdentityCombiner, DefaultAnnSortedMultiKeysKeyIoModel>;
using TOffroadAnnKeyWadIo = TOffroadKeyWadIo<AnnIndexType, ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor, NOffroad::TNullSerializer, TIdentityCombiner, DefaultAnnSortedMultiKeysKeyIoModel>;
using TOffroadLinkAnnKeyWadIo = TOffroadKeyWadIo<LinkAnnIndexType, ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor, NOffroad::TNullSerializer, TIdentityCombiner, DefaultAnnSortedMultiKeysKeyIoModel>;

} // namespace NDoom
