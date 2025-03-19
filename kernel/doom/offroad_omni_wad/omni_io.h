#pragma once

#include <kernel/doom/offroad_struct_wad/offroad_struct_wad_io.h>
#include <kernel/doom/offroad_struct_wad/serializers.h>

namespace NDoom {
    using TOmniUrlIo = TOffroadStructWadIo<OmniUrlType, TStringBuf, TStringBufSerializer, AutoEofStructType, OffroadCompressionType, DefaultOmniUrlIoModel>;
    using TOmniTitleIo = TOffroadStructWadIo<OmniTitleType, TStringBuf, TStringBufSerializer, AutoEofStructType, OffroadCompressionType, DefaultOmniTitleIoModel>;
    using TOmniDupUrlsIo = TOffroadStructWadIo<OmniDupUrlsType, TStringBuf, TStringBufSerializer, AutoEofStructType>;
    using TOmniEmbedUrlsIo = TOffroadStructWadIo<OmniEmbedUrlsType, TStringBuf, TStringBufSerializer, AutoEofStructType>;

    using TOmniDssmLogDwellTimeBigramsEmbeddingIo = TOffroadStructWadIo<OmniDssmEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmLogDwellTimeBigramsEmbeddingIoModel>;
    using TOmniAnnRegStatsIo = TOffroadStructWadIo<OmniAnnRegStatsType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniAnnRegStatsIoModel>;
    using TOmniDssmAggregatedAnnRegEmbeddingIo = TOffroadStructWadIo<OmniDssmAggregatedAnnRegEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAggregatedAnnRegEmbeddingIoModel>;
    using TOmniDssmAnnCtrCompressedEmbeddingIo = TOffroadStructWadIo<OmniDssmAnnCtrCompressedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAnnCtrCompressedEmbeddingIoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding1Io = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding1Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding1IoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding2Io = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding2Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding2IoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding3Io = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding3Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding3IoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding4Io = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding4Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding4IoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding5Io = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding5Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding5IoModel>;
    using TOmniDssmAnnXfDtShowOneCompressedEmbeddingIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowOneCompressedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAnnXfDtShowOneCompressedEmbeddingIoModel>;
    using TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowOneSeCompressedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmAnnXfDtShowOneSeCompressedEmbeddingIoModel>;
    using TOmniDssmMainContentKeywordsEmbeddingIo = TOffroadStructWadIo<OmniDssmMainContentKeywordsEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, OffroadCompressionType, DefaultOmniDssmMainContentKeywordsEmbeddingIoModel>;


    using TOmniDssmLogDwellTimeBigramsEmbeddingRawIo = TOffroadStructWadIo<OmniDssmEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniAnnRegStatsRawIo = TOffroadStructWadIo<OmniAnnRegStatsType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAggregatedAnnRegEmbeddingRawIo = TOffroadStructWadIo<OmniDssmAggregatedAnnRegEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnCtrCompressedEmbeddingRawIo = TOffroadStructWadIo<OmniDssmAnnCtrCompressedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding1RawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding1Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding2RawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding2Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding3RawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding3Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding4RawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding4Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbedding5RawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbedding5Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowWeightCompressedEmbeddingsRawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowWeightCompressedEmbeddingsType, TArrayRef<const char>, TDataRegionSerializer, VariableSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowOneCompressedEmbeddingRawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowOneCompressedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingRawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowOneSeCompressedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmMainContentKeywordsEmbeddingRawIo = TOffroadStructWadIo<OmniDssmMainContentKeywordsEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnXfDtShowOneSeAmSsHardCompressedEmbeddingRawIo = TOffroadStructWadIo<OmniDssmAnnXfDtShowOneSeAmSsHardCompressedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmPantherTermsEmbeddingRawIo = TOffroadStructWadIo<OmniDssmPantherTermsEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmAnnSerpSimilarityHardSeCompressedEmbeddingRawIo = TOffroadStructWadIo<OmniDssmAnnSerpSimilarityHardSeCompressedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniAliceWebMusicTrackTitleIo = TOffroadStructWadIo<OmniAliceWebMusicTrackTitleType, TStringBuf, TStringBufSerializer, AutoEofStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniAliceWebMusicArtistNameIo = TOffroadStructWadIo<OmniAliceWebMusicArtistNameType, TStringBuf, TStringBufSerializer, AutoEofStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniRelCanonicalTargetIo = TOffroadStructWadIo<OmniRelCanonicalTargetType, TStringBuf, TStringBufSerializer, AutoEofStructType, RawCompressionType, NoStandardIoModel>;

    using TOmniRecDssmSpyTitleDomainCompressedEmb12RawIo = TOffroadStructWadIo<OmniRecDssmSpyTitleDomainCompressedEmb12Type, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniRecCFSharpDomainRawIo = TOffroadStructWadIo<OmniRecCFSharpDomainType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;

    using TOmniDssmFpsSpylogAggregatedEmbeddingRawIo = TOffroadStructWadIo<OmniDssmFpsSpylogAggregatedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;
    using TOmniDssmUserHistoryHostClusterEmbeddingRawIo = TOffroadStructWadIo<OmniDssmUserHistoryHostClusterEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;

    using TOmniDssmBertDistillL2EmbeddingRawIo = TOffroadStructWadIo<OmniDssmBertDistillL2EmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;

    using TOmniDssmNavigationL2EmbeddingRawIo = TOffroadStructWadIo<OmniDssmNavigationL2EmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;

    using TOmniDssmFullSplitBertEmbeddingRawIo = TOffroadStructWadIo<OmniDssmFullSplitBertEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;

    using TOmniReservedEmbeddingRawIo = TOffroadStructWadIo<OmniReservedEmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;

    using TOmniDssmSinsigL2EmbeddingRawIo = TOffroadStructWadIo<OmniDssmSinsigL2EmbeddingType, TArrayRef<const char>, TDataRegionSerializer, FixedSizeStructType, RawCompressionType, NoStandardIoModel>;

} // namespace NDoom
