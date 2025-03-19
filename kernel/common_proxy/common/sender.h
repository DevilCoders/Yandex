#pragma once

#include "processor.h"

namespace NCommonProxy {

    class TSender : public TProcessor {
    public:
        TSender(const TString& name, const TProcessorsConfigs& configs);
    };
}
