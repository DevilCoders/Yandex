#pragma once

#include <util/generic/vector.h>

namespace NSnippets {
    class TQueryy;

    class TQueryPosSqueezer {
    private:
        const size_t Threshold = 0;
        TVector<int> Pos2SqueezedPos;
        TVector<double> Weights;

        void SqueezeWeights(const TQueryy& query);

    public:
        TQueryPosSqueezer(const TQueryy& query, const size_t threshold);

        const TVector<double>& GetSqueezedWeights() const;
        size_t GetSqueezedPosCount() const;
        TVector<bool> Squeeze(const TVector<bool>& seenPos) const;
        TVector<bool> Squeeze(const TVector<int>& seenPos) const;
        size_t GetThreshold() const;
    };
}
