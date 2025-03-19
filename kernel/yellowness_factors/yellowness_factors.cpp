#include "yellowness_factors.h"

namespace NYellownessFactors {

    namespace {

        class TFactorNamesSetter {
        public:
            TFactorNamesSetter() {
                const auto infoPtr = GetWebFactorsInfo();
                for (const auto idx : xrange(FactorIndices.size())) {
                    Y_ASSERT(FactorIndices[idx] < infoPtr->GetFactorCount());
                    FactorNames_[idx] = infoPtr->GetFactorName(FactorIndices[idx]);
                }
            }

            const TFactorNames& GetFactorNames() const noexcept {
                return FactorNames_;
            }

        private:
            TFactorNames FactorNames_;
        };

    } // unnamed namespace

    const TFactorNames& GetFactorsNames() {
        return Singleton<TFactorNamesSetter>()->GetFactorNames();
    }

} // NYellownessFactors
