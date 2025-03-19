#pragma once

#include <kernel/feature_pool/feature_pool.h>

class TOneFactorInfo;
class IFactorsInfo;

namespace NFactorSlices {
    class TFactorDomain;
}

namespace NRankingFeaturePool {
    using namespace NMLPool;

    void MakeAbsentFeatureInfo(const TString& sliceName, TFeatureInfo& res);
    void MakeFeatureInfo(const TOneFactorInfo& info, TFeatureInfo& res);
    void MakeFeatureInfo(const IFactorsInfo& info, size_t index, TFeatureInfo& res);
    void MakePoolInfo(const NFactorSlices::TFactorDomain& domain,
                      TPoolInfo& res);
} // NRankingFeaturePool
