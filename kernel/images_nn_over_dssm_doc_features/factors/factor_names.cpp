#include "factor_names.h"

#include <kernel/generated_factors_info/simple_factors_info.h>
// autogenerated code
#include <kernel/images_nn_over_dssm_doc_features/factors/factors_gen.cpp>

#include <util/datetime/base.h>
#include <util/generic/singleton.h>

namespace NImagesNnOverDssmDocFeatures {

    // This function is declared in code-generated header factors_gen.h
    TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
        Y_ASSERT(begin <= end);
        Y_ASSERT(end <= N_FACTOR_COUNT);
        return new TWebFactorsInfo<TFactorInfo>(begin, end, NImagesNnOverDssmDocFeatures::GetFactorsInfo());
    }

    float CalcTimeFactorDays(const TInstant& requestTime, const TInstant& docTime) {
        if (docTime > requestTime) {
            return 0.f;
        }
        const float age = static_cast<float>((requestTime - docTime).Days());
        return age / (1.f + age);
    }

    float CalcTimeFactorMinutes(const TInstant& requestTime, const TInstant& docTime) {
        if (docTime > requestTime) {
            return 0.f;
        }
        const auto ageInMinutes = (requestTime - docTime).Minutes();
        return static_cast<float>(ageInMinutes) / (ageInMinutes + TDuration::Days(1).Minutes());
    }

    float ImageAreaSqrtSigmoid(float area) {
        area = std::sqrt(area);
        area = (area - 500.0f) / 100.0f;

        area = Max(-4.0f, area);
        area = Min(4.0f, area);

        return 1.0f / (1.0f + std::exp(-area));
    }

    float ImageAreaSigmoid(float area) {
        return 1.0f / (1.0f + std::exp(3.0f - area * 0.00001f));
    }


    class TImagesNnOverDssmFactorsInfo : public TSimpleSearchFactorsInfo<NImagesNnOverDssmDocFeatures::TFactorInfo> {
    public:
        TImagesNnOverDssmFactorsInfo(size_t begin, size_t end, const NImagesNnOverDssmDocFeatures::TFactorInfo* factors)
            : TSimpleSearchFactorsInfo<NImagesNnOverDssmDocFeatures::TFactorInfo>(end - begin, factors + begin)
        {
        }

        TImagesNnOverDssmFactorsInfo()
            : TImagesNnOverDssmFactorsInfo(0, NImagesNnOverDssmDocFeatures::FI_FACTOR_COUNT, NImagesNnOverDssmDocFeatures::GetFactorsInfo())
        {
        }

    };

    const IFactorsInfo* GetImagesFactorsInfo() {
        return Singleton<NImagesNnOverDssmDocFeatures::TImagesNnOverDssmFactorsInfo>();
    }
}
