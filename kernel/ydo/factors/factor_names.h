#pragma once

#include <util/system/defaults.h>

// generated from factors_gen.in
#include <kernel/ydo/factors/factors_gen.h>

class IFactorsInfo;

namespace NYdo::NBegemotFactors {
    TAutoPtr<IFactorsInfo> GetBegemotFactorsInfo();
    size_t GetFactorIndex(const char* name);
}
