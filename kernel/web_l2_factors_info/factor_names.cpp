#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>


namespace NWebL2 {

class TWebL2FactorsInfo: public TSimpleSearchFactorsInfo<NWebL2::TFactorInfo> {
public:
    TWebL2FactorsInfo()
        : TSimpleSearchFactorsInfo<NWebL2::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
    {
    }

    TWebL2FactorsInfo(size_t begin, size_t end, const NWebL2::TFactorInfo* factors)
        : TSimpleSearchFactorsInfo<NWebL2::TFactorInfo>(end - begin, factors + begin)
    {
    }
};

const IFactorsInfo* GetWebL2FactorsInfo() {
    return Singleton<TWebL2FactorsInfo>();
}

TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
    Y_ASSERT(begin <= end);
    Y_ASSERT(end <= FI_FACTOR_COUNT);
    return new TWebL2FactorsInfo(begin, end, GetFactorsInfo());
}

} // namespace NWebL2
