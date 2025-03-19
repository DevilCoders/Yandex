#pragma once
#include <util/system/types.h>

namespace NDssmApplier {
    enum class EDssmDocEmbedding : ui32 {
        LogDTBigramsEmbedding = 0,
        LogDTBigramsAMHardsEmbedding = 1,
        DssmBoostingXfDtShowOneSeAmSsHardEmbedding = 2,
        DwellTimeRegChainEmbedding = 3,
        QueryWordTitleEmbedding = 4,
        LogDTBigramsEmbeddingL2 = 5,
        PantherTermsEmbedding = 6,
        CompressedUserRecDssmSpyTitleDomain = 7,
        CompressedUrlRecDssmSpyTitleDomain = 8,
        UncompressedCFSharp = 9,
        ReformulationsLongestClickLogDtEmbedding = 10,
        DistillSinsigMseBaseRegChainEmbedding = 11,
        DistillRelevanceMseBaseRegChainEmbedding = 12,
    };

    enum class EDssmDocEmbeddingType : ui8 {
        Uncompressed = 0,
        Compressed = 1,
        CompressedWithWeights = 2,
        CompressedLogDwellTimeL2 = 3,
        CompressedPantherTerms = 4,
        CompressedUserRecDssmSpyTitleDomain = 5,
        CompressedUrlRecDssmSpyTitleDomain = 6,
        UncompressedCFSharp = 7,
    };
}
