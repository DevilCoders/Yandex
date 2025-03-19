#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>

namespace NWebItdItp {

class TWebItdItpFactorsInfo: public TSimpleSearchFactorsInfo<NWebItdItp::TFactorInfo> {
public:
    TWebItdItpFactorsInfo()
        : TSimpleSearchFactorsInfo<NWebItdItp::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
    {
    }

    TWebItdItpFactorsInfo(size_t begin, size_t end, const NWebItdItp::TFactorInfo* factors)
        : TSimpleSearchFactorsInfo<NWebItdItp::TFactorInfo>(end - begin, factors + begin)
    {
    }
};

const IFactorsInfo* GetWebItdItpFactorsInfo() {
    return Singleton<TWebItdItpFactorsInfo>();
}

TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
    Y_ASSERT(begin <= end);
    Y_ASSERT(end <= NWebItdItp::FI_FACTOR_COUNT);
    return new TWebItdItpFactorsInfo(begin, end, GetFactorsInfo());
}

} // namespace NWebItdItp
