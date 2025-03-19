#include "factor_names.h"

#include <util/generic/hash.h>
#include <util/generic/singleton.h>

#include <kernel/factors_info/factors_info.h>
#include <kernel/generated_factors_info/simple_factors_info.h>

// generated from factors_gen.in
#include <kernel/ydo/factors/factors_gen.cpp>

namespace NYdo {
    namespace NBegemotFactors {
        class TBegemotFactorsInfo: public TSimpleSearchFactorsInfo<NYdo::NBegemotFactors::TFactorInfo> {
        public:
            TBegemotFactorsInfo(size_t begin, size_t end, const NYdo::NBegemotFactors::TFactorInfo* factors)
                : TSimpleSearchFactorsInfo<NYdo::NBegemotFactors::TFactorInfo>(end - begin, factors + begin)
            {
            }
            TBegemotFactorsInfo()
                : TBegemotFactorsInfo(0, NYdo::NBegemotFactors::FI_FACTOR_COUNT, NYdo::NBegemotFactors::GetFactorsInfo())
            {
            }
        };

        TAutoPtr<IFactorsInfo> GetBegemotFactorsInfo() {
            return new TBegemotFactorsInfo();
        }

        size_t GetFactorIndex(const char* name) {
            size_t result = NYdo::NBegemotFactors::FI_FACTOR_COUNT;
            if (GetBegemotFactorsInfo()->GetFactorIndex(name, &result)) {
                return result;
            }
            return (GetBegemotFactorsInfo()->GetFactorCount() + 1);
        }

        TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end) {
            Y_ASSERT(begin <= end);
            Y_ASSERT(end <= NYdo::NBegemotFactors::FI_FACTOR_COUNT);
            return new TBegemotFactorsInfo(begin, end, NYdo::NBegemotFactors::GetFactorsInfo());
        }
    }
}
