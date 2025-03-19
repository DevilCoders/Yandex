#pragma once

#include "template_stuff.h"
#include "omnidoc_fwd.h"

namespace NIndexDoc {

/* This mapping is used to move fields and keep compatibility
 *
 * firstly std::get<Io>(docId, accessors) wil try to use accessor for Io, if it is not inited,
 * we gonna try to look at this mapping. If it has Io at fist place of any line,
 * others we shall try other Ios accessors one by one. So you can put any number of Ios here
 */

    using TCompatibilityMapping = TTypeList<
        TTypeList<NDoom::TOmniDssmLogDwellTimeBigramsEmbeddingRawIo, NDoom::TOmniDssmLogDwellTimeBigramsEmbeddingIo>,
        TTypeList<NDoom::TOmniAnnRegStatsRawIo, NDoom::TOmniAnnRegStatsIo>,
        TTypeList<NDoom::TOmniDssmAggregatedAnnRegEmbeddingRawIo, NDoom::TOmniDssmAggregatedAnnRegEmbeddingIo>,
        TTypeList<NDoom::TOmniDssmMainContentKeywordsEmbeddingRawIo, NDoom::TOmniDssmMainContentKeywordsEmbeddingIo>,
        TTypeList<NDoom::TOmniDssmAnnXfDtShowOneCompressedEmbeddingRawIo, NDoom::TOmniDssmAnnXfDtShowOneCompressedEmbeddingIo>,
        TTypeList<NDoom::TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingRawIo, NDoom::TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingIo>,
        TTypeList<NDoom::TOmniDssmAnnCtrCompressedEmbeddingRawIo, NDoom::TOmniDssmAnnCtrCompressedEmbeddingIo>,

        // identity mappings: not used anymore, remove with old omni-index
        TTypeList<NDoom::TOmniDssmLogDwellTimeBigramsEmbeddingRawIo, NDoom::TOmniDssmLogDwellTimeBigramsEmbeddingRawIo>,
        TTypeList<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding1Io, NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding1Io>,
        TTypeList<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding2Io, NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding2Io>,
        TTypeList<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding3Io, NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding3Io>,
        TTypeList<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding4Io, NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding4Io>,
        TTypeList<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding5Io, NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding5Io>
    >;

    template <class Io>
    struct THasCompatibilityLayers {
        static constexpr bool Value = TTypeListHasMatchingHead<Io, NIndexDoc::TCompatibilityMapping>::Value;
        using TType = std::integral_constant<bool, Value>;
    };

    template <class Io>
    struct TCompatibilityLayers {
        using TType = typename TMatchTypeListInTypeListByHead<Io, NIndexDoc::TCompatibilityMapping>::TType;
    };


    template<class Io, class _ = void>
    struct TCompatibilityLayersWithHead;

    template<class Io>
    struct TCompatibilityLayersWithHead<Io, std::enable_if_t<(typename NIndexDoc::THasCompatibilityLayers<Io>::TType())>> {
        using TType = typename NTL::TConcat<TTypeList<Io>, typename NIndexDoc::TCompatibilityLayers<Io>::TType>::type;
    };

    template<class Io>
    struct TCompatibilityLayersWithHead<Io, std::enable_if_t<(!typename NIndexDoc::THasCompatibilityLayers<Io>::TType())>> {
        using TType = TTypeList<Io>;
    };
}
