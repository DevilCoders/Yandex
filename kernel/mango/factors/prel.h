#pragma once

#include <cstdlib>

namespace NMango {

    class TPositionalRelevance {
    public:
        static const float MAGIC_K1;

        TPositionalRelevance()
            : HitWeightSum(0.0f)
        {
        }

        void OnResourceStart();
        void AddHit(float idf, size_t hitPos);
        float GetRelevance(size_t quoteCount) const;

    private:
        float HitWeightSum;
    };

}
