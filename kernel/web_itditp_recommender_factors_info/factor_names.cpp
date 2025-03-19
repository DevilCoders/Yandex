#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>

namespace NWebItdItpRecommender {

class TWebItdItpRecommenderFactorsInfo: public TSimpleSearchFactorsInfo<NWebItdItpRecommender::TFactorInfo> {
public:
    TWebItdItpRecommenderFactorsInfo()
        : TSimpleSearchFactorsInfo<NWebItdItpRecommender::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
    {
    }

    TWebItdItpRecommenderFactorsInfo(size_t begin, size_t end, const NWebItdItpRecommender::TFactorInfo* factors)
        : TSimpleSearchFactorsInfo<NWebItdItpRecommender::TFactorInfo>(end - begin, factors + begin)
    {
    }
};

const IFactorsInfo* GetWebItdItpRecommenderFactorsInfo() {
    return Singleton<TWebItdItpRecommenderFactorsInfo>();
}

TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
    Y_ASSERT(begin <= end);
    Y_ASSERT(end <= N_FACTOR_COUNT);
    return new TWebItdItpRecommenderFactorsInfo(begin, end, GetFactorsInfo());
}

}  // namespace NWebItdItpRecommender
