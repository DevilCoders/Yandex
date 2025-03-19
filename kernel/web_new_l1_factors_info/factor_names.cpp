#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>

namespace NWebNewL1 {

class TWebNewL1FactorsInfo: public TSimpleSearchFactorsInfo<NWebNewL1::TFactorInfo> {
public:
    TWebNewL1FactorsInfo()
        : TSimpleSearchFactorsInfo<NWebNewL1::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
    {
    }

    TWebNewL1FactorsInfo(size_t begin, size_t end, const NWebNewL1::TFactorInfo* factors)
        : TSimpleSearchFactorsInfo<NWebNewL1::TFactorInfo>(end - begin, factors + begin)
    {
    }
};

const IFactorsInfo* GetWebNewL1FactorsInfo() {
    return Singleton<TWebNewL1FactorsInfo>();
}

TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
    Y_ASSERT(begin <= end);
    Y_ASSERT(end <= FI_FACTOR_COUNT);
    return new TWebNewL1FactorsInfo(begin, end, GetFactorsInfo());
}

} // namespace NWebNewL1
