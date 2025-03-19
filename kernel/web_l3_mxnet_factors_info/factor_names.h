#pragma once

#include <kernel/web_l3_mxnet_factors_info/factors_gen.h>

class IFactorsInfo;

namespace NWebL3Mx {

    const IFactorsInfo* GetWebTransFeaturesInfo();

} // namespace NWebL3Mx
