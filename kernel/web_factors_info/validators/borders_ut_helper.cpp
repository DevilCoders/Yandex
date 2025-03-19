#include "borders_ut_helper.h"

#include <kernel/factors_info/factors_info.h>
#include <util/generic/xrange.h>
#include <kernel/factor_slices/slices_info.h>

void NFactorsInfoValidators::CheckBorders(const IFactorsInfo* factors) {
    for (size_t i : xrange(factors->GetFactorCount())) {
        float minValue = factors->GetMinValue(i);
        float maxValue = factors->GetMaxValue(i);

        Y_ENSURE(minValue <= maxValue,
            "factor " << factors->GetFactorName(i) << " has incorrect interval [" << minValue << ", " << maxValue << "]"
        );

        if (factors->IsNot01Factor(i)) {
            Y_ENSURE(!(minValue == 0 && maxValue == 1),
                "factor " << factors->GetFactorName(i) << " declared as TG_NOT_01 but it is 01"
            );
        }

        if (!factors->IsNot01Factor(i)) {
            Y_ENSURE(minValue == 0 && maxValue == 1,
                "factor " << factors->GetFactorName(i) <<
                " not declared as TG_NOT_01 but it is has intervals [" << minValue << ", " << maxValue << "]"
            );
        }
    }
}
