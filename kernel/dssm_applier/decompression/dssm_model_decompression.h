#pragma once

#include <util/system/types.h>

namespace NDssmApplier {
    enum class EDssmModelType: ui32 {
        LogDwellTimeBigrams = 0,
        MarketMiddleClick = 1,
        AnnRegStats = 2, // TODO(olegator@): better naming, stats are not DSSM
        ConcatenatedAnnReg = 3, // deprecated
        AggregatedAnnReg = 4,
        DssmBoostingWeights = 5,
        DssmBoostingNorm = 6,
        DssmBoostingEmbeddings = 7,
        MainContentKeywords = 8,
        MarketHard = 9,
        ReducedConcatenatedEmbeddings = 10,
        PantherTerms = 11,
        DwelltimeBigrams = 12,
        DwelltimeMulticlick = 13,
        LogDtBigramsAmHardQueriesNoClicks = 14,
        LogDtBigramsAmHardQueriesNoClicksMixed = 15,

        // ITDITP-550
        // These bounds are for NDJ::NWeb::ET_NewHistoryDssm which consists of two parts: url and user
        RecDssmSpyTitleDomainUrl = 16,  // url part
        RecDssmSpyTitleDomainUser = 17,  // user part
        FpsSpylogAggregatedQueryPart = 18, // query part
        FpsSpylogAggregatedDocPart = 19, // doc part
        UserHistoryHostClusterDocPart = 20, // doc part
        ReformulationsLongestClickLogDt = 21,

        LogDtBigramsQueryPart = 22,
        MarketHard2Bert = 23,
        MarketReformulation = 24,
        ReformulationsQueryEmbedderMini = 25,
        MarketSuperEmbed = 26,
        AssessmentBinary = 27,
        Assessment = 28,
        Click = 29,
        HasCpaClick = 30,
        Cpa = 31,
        BilledCpa = 32,
        MarketImage2TextV10 = 33
    };
}
