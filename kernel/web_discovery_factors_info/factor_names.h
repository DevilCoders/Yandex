#pragma once

#include <kernel/web_discovery_factors_info/factors_gen.h>

class IFactorsInfo;

namespace NWebDiscovery {
    const IFactorsInfo* GetWebDiscoveryFactorsInfo();
}
