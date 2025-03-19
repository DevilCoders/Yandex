#pragma once

#include <kernel/web_l1_factors_info/factors_gen.h>

class IFactorsInfo;

namespace NWebL1 {

const IFactorsInfo* GetWebL1FactorsInfo();

} // namespace NWebL1
