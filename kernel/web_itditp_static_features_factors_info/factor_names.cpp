#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>

namespace NWebItdItpStaticFeatures {

class TWebItdItpStaticFeaturesFactorsInfo: public TSimpleSearchFactorsInfo<NWebItdItpStaticFeatures::TFactorInfo> {
public:
    TWebItdItpStaticFeaturesFactorsInfo()
        : TSimpleSearchFactorsInfo<NWebItdItpStaticFeatures::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
    {
    }

    TWebItdItpStaticFeaturesFactorsInfo(size_t begin, size_t end, const NWebItdItpStaticFeatures::TFactorInfo* factors)
        : TSimpleSearchFactorsInfo<NWebItdItpStaticFeatures::TFactorInfo>(end - begin, factors + begin)
    {
    }
};

const IFactorsInfo* GetWebItdItpStaticFeaturesFactorsInfo() {
    return Singleton<TWebItdItpStaticFeaturesFactorsInfo>();
}

TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
    Y_ASSERT(begin <= end);
    Y_ASSERT(end <= N_FACTOR_COUNT);
    return new TWebItdItpStaticFeaturesFactorsInfo(begin, end, GetFactorsInfo());
}

}  // namespace NWebItdItpStaticFeatures
