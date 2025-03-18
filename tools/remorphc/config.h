#pragma once

#include "unit_config.h"

#include <util/system/defaults.h>

namespace NRemorphCompiler {

class TConfig {
private:
    TUnits Units;

public:
    TConfig();

    inline const TUnits& GetUnits() const {
        return Units;
    }

    inline TUnits& GetUnits() {
        return Units;
    }

    inline size_t Size() const {
        return Units.size();
    }

    inline bool Empty() const {
        return Units.empty();
    }
};

} // NRemorphCompiler
