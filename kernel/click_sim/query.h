#pragma once

#include "word_vector.h"

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

#include <optional>

namespace NClickSim {
    TString Query2SaasKey(const TString& normalizedQuery);

    /**
     * Construct pseudo click similarity vector from Ling Boost expansions.
     * (make a vector from each expansion and sum them up with its weights)
     */
    std::optional<TWordVector> MakeVectorFromLingBoostExpansions(const TVector<std::pair<TString, float>>& expansions,
        size_t stripTo, const THashSet<TString>* stopWords = nullptr);
}
