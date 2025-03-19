#pragma once

#include <kernel/dssm_applier/decompression/dssm_model_decompression.h_serialized.h>
#include <kernel/dssm_applier/utils/utils.h>
#include <util/system/types.h>
#include <util/generic/cast.h>

#include <utility>

namespace NDssmApplier {

    namespace {
        using NUtils::GenerateSingleDecompression;
        using NUtils::MakeArray;

        constexpr std::array<std::pair<float, float>, GetEnumItemsCount<EDssmModelType>()> DSSM_MODEL_TO_BOUNDS
            {{
                 {-0.555593, 0.541551},          // LogDwellTimeBigrams
                 {-0.50041, 0.381908},           // MarketMiddleClick
                 {0.0, 1.0},                     // AnnRegStats
                 {-0.979518, 0.648810},          // ConcatenatedAnnReg
                 {-0.895084, 0.814993},          // AggregatedAnnReg
                 {0.0, 1.0},                     // DssmBoostingWeights
                 {0.0, 0.13},                    // DssmBoostingNorm
                 {-1.0, 1.0},                    // DssmBoostingEmbeddings
                 {-0.803529, 0.643836},          // MainContentKeywords
                 {-0.96, 0.59},                  // MarketHard
                 {-0.651886, 0.570615},          // ReducedConcatenatedEmbeddings
                 {-0.651886, 0.570615},          // PantherTerms
                 {-0.630580, 0.519844},          // DwelltimeBigrams
                 {-0.675159, 0.404499},          // DwelltimeMulticlick
                 {-0.790291, 0.497332},          // LogDtBigramsAmHardQueriesNoClicks
                 {-0.980184, 0.338241},          // LogDtBigramsAmHardQueriesNoClicksMixed
                 {-0.6526937f, 0.4913636f},      // RecDssmSpyTitleDomainUrl
                 {-0.5284958f, 0.3981255f},      // RecDssmSpyTitleDomainUser
                 {-0.9213974f, 0.5437454f},      // FpsSpylogAggregatedQueryPart
                 {-0.6388595f, 0.3244397f},      // FpsSpylogAggregatedDocPart
                 {-0.6592008f, 0.5915975f},      // UserHistoryHostClusterDocPart
                 {-0.5679786f, 0.9236386f},      // ReformulationsLongestClickLogDt (compressed model)
                 {-0.9616003f, 0.6536958f},      // LogDtBigramsQueryPart (after begemot compression and normalization)
                 {-0.94, 0.78},                  // MarketHard2Bert
                 {-0.51, 0.84},                  // MarketReformulation
                 {-0.9406866f, 0.9283971f},      // ReformulationsQueryEmbedderMini
                 {-0.66058874, 0.66445214},      // MarketSuperEmbed
                 {-0.72, 0.97},                  // AssessmentBinary
                 {-0.75, 0.99},                  // Assessment
                 {-1.0, 0.55},                   // Click
                 {-0.8, 0.81},                   // HasCpaClick
                 {-0.86, 0.88},                  // Cpa
                 {-0.87, 0.89},                  // BilledCpa
                 {-0.197, 0.1937},               // MarketImage2TextV10

            }};

        constexpr TDecompression GetDecomressionForModel(size_t index) noexcept {
            return GenerateSingleDecompression(DSSM_MODEL_TO_BOUNDS[index].first, DSSM_MODEL_TO_BOUNDS[index].second);
        }

        constexpr std::array<TDecompression, GetEnumItemsCount<EDssmModelType>()> GenerateDecompressions() noexcept {
            return MakeArray<GetEnumItemsCount<EDssmModelType>()>(GetDecomressionForModel);
        }

        constexpr std::array<TDecompression, GetEnumItemsCount<EDssmModelType>()> DECOMPRESSIONS = GenerateDecompressions();

    } // unnamed namespace

    constexpr const TDecompression& GetDecompression(EDssmModelType modelType) noexcept {
        return DECOMPRESSIONS[(ToUnderlying(modelType))];
    }

    constexpr std::pair<float, float> GetBounds(EDssmModelType modelType) noexcept {
        return DSSM_MODEL_TO_BOUNDS[ToUnderlying(modelType)];
    }

    TVector<ui8> Compress(const TArrayRef<const float>& numbers, EDssmModelType modelType) noexcept;
    TVector<float> Decompress(const TString& embedding, EDssmModelType modelType) noexcept;
    TVector<i8> Scale(const TArrayRef<const float>& numbers, EDssmModelType modelType) noexcept;
    TVector<float> UnScale(const TArrayRef<const i8>& numbers, EDssmModelType modelType) noexcept;

}  // namespace NDssmApplier
