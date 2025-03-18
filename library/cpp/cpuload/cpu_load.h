#pragma once

#include <util/system/defaults.h>

namespace NCpuLoad {
    struct TInfo {
        ui32 User;
        ui32 System;
    };

    TInfo Get();
}
