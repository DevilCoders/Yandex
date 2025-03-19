#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>

namespace NWebL1 {

class TWebL1FactorsInfo: public TSimpleSearchFactorsInfo<NWebL1::TFactorInfo> {
public:
    TWebL1FactorsInfo()
        : TSimpleSearchFactorsInfo<NWebL1::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
    {
    }

    TWebL1FactorsInfo(size_t begin, size_t end, const NWebL1::TFactorInfo* factors)
        : TSimpleSearchFactorsInfo<NWebL1::TFactorInfo>(end - begin, factors + begin)
    {
    }
};

const IFactorsInfo* GetWebL1FactorsInfo() {
    return Singleton<TWebL1FactorsInfo>();
}

TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
    Y_ASSERT(begin <= end);
    Y_ASSERT(end <= FI_FACTOR_COUNT);
    return new TWebL1FactorsInfo(begin, end, GetFactorsInfo());
}

} // namespace NWebL1
