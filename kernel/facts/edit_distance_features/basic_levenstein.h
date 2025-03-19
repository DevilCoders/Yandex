#pragma once

#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

#include <util/generic/algorithm.h>

#include <utility>

namespace NEditDistanceFeatures {

    template<typename TSeq>
    std::pair<float, size_t> GetLevensteinDistance(const TSeq& lhs, const TSeq& rhs) {
        if (lhs.empty() && rhs.empty()) {
            return std::make_pair(0.0f, 0);
        }

        NLevenshtein::TEditChain chain;
        float distance = 0.0f;
        NLevenshtein::GetEditChain(lhs, rhs, chain, &distance);

        return std::make_pair(distance, chain.size());
    }


}
