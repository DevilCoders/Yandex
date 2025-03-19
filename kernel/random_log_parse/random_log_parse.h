#pragma once

#include <kernel/factor_slices/factor_domain.h>
#include <kernel/factor_slices/factor_slices.h>

#include <library/cpp/json/writer/json_value.h>

/*
 * Appends ranking factors to the end of rankingFactors vector OR
 *   fills rankingFactors vector from scratch (details see in impl).
 */
bool ExtractRankingFactors(const NJson::TJsonValue& randomLogData,
    TVector<float>& rankingFactors,
    const NFactorSlices::EFactorSlice slice = NFactorSlices::EFactorSlice::WEB);

bool ExtractRankingFactorsFromMarker(const TStringBuf randomLogMarker,
    TVector<float>& rankingFactors,
    const NFactorSlices::EFactorSlice slice = NFactorSlices::EFactorSlice::WEB);

/*
 * Following functions are used to parse more than one factor slice
 *   aligning them with respect to the factor domain.
 * To use these functions you need to link your binary with code
 *   generated factors info for each used factor slice.
 * Clears rankingFactors vector before filling.
 */
bool FillRankingFactorsFromRandomLogMarker(const TStringBuf randomLogMarker,
    const NFactorSlices::TFactorDomain& factorDomain,
    TVector<float>& rankingFactors);

bool FillRankingFactorsFromRandomLog(const NJson::TJsonValue& randomLogData,
    const NFactorSlices::TFactorDomain& factorDomain,
    TVector<float>& rankingFactors);
