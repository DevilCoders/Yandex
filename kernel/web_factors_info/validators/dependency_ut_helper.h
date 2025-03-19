#pragma once
#include <util/system/types.h>

class IFactorsInfo;

namespace NFactorsInfoValidators {
    void CheckDependenciesList(const IFactorsInfo* factors) noexcept(false);
}
