#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>


namespace NWebL3Mx {

    class TWebTransFeaturesInfo: public TSimpleSearchFactorsInfo<NWebL3Mx::TFactorInfo> {
    public:
        TWebTransFeaturesInfo()
            : TSimpleSearchFactorsInfo<NWebL3Mx::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
        {
        }

        TWebTransFeaturesInfo(size_t begin, size_t end, const NWebL3Mx::TFactorInfo* factors)
            : TSimpleSearchFactorsInfo<NWebL3Mx::TFactorInfo>(end - begin, factors + begin)
        {
        }
    };

    const IFactorsInfo* GetWebTransFeaturesInfo() {
        return Singleton<TWebTransFeaturesInfo>();
    }

    TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
        Y_ASSERT(begin <= end);
        Y_ASSERT(end <= FI_FACTOR_COUNT);
        return new TWebTransFeaturesInfo(begin, end, GetFactorsInfo());
    }

} // namespace NWebL3Mx
