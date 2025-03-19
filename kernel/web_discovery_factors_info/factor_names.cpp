#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>

namespace NWebDiscovery {

class TWebDiscoveryFactorsInfo: public TSimpleSearchFactorsInfo<NWebDiscovery::TFactorInfo> {
public:
    TWebDiscoveryFactorsInfo()
        : TSimpleSearchFactorsInfo<NWebDiscovery::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
    {
    }

    TWebDiscoveryFactorsInfo(size_t begin, size_t end, const NWebDiscovery::TFactorInfo* factors)
        : TSimpleSearchFactorsInfo<NWebDiscovery::TFactorInfo>(end - begin, factors + begin)
    {
    }
};

const IFactorsInfo* GetWebDiscoveryFactorsInfo() {
    return Singleton<TWebDiscoveryFactorsInfo>();
}

TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
    Y_ASSERT(begin <= end);
    Y_ASSERT(end <= NWebDiscovery::FI_FACTOR_COUNT);
    return new TWebDiscoveryFactorsInfo(begin, end, GetFactorsInfo());
}

} // namespace NWebDiscovery
