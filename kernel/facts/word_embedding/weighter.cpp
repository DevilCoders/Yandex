#include "weighter.h"

namespace NUnstructuredFeatures {
    namespace NWordEmbedding {

        void TSIFWeighter::Weight(const TVector<TAnalyzedWord>& input, TVector<float>& weights) const {
            weights.reserve(input.size());
            for (const TAnalyzedWord& analyzedWord : input) {
                weights.push_back(Alpha / (Alpha + analyzedWord.Frequency));
            }
        }
    }
}
