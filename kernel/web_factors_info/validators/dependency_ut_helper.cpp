#include "dependency_ut_helper.h"

#include <kernel/factors_info/factors_info.h>
#include <util/generic/xrange.h>
#include <kernel/factor_slices/slices_info.h>

void NFactorsInfoValidators::CheckDependenciesList(const IFactorsInfo* factors) noexcept(false) {
     THashMap<TString, TVector<TString>> depsList;
    for (size_t i : xrange(factors->GetFactorCount())) {
        depsList.clear();
        factors->GetDependencyNames(i, depsList);
        if (!depsList.empty()) {
            for (auto& [k, v] : depsList) {
                NFactorSlices::EFactorSlice depSliceName = FromString(k);
                const IFactorsInfo* depFactorsInfo = NFactorSlices::GetSlicesInfo()->GetFactorsInfo(depSliceName);
                Y_ENSURE(depFactorsInfo != nullptr, "no info for " << k);
                for(auto& vv : v) {
                    Y_ENSURE(depFactorsInfo->GetFactorIndex(vv.c_str()), "unknown factor: " << vv << " in slice " << k);
                }
            }
        }
    }
}
