#pragma once

#include "calculator_config.h"
#include <mapreduce/yt/interface/operation.h>

namespace NUnstructuredFeatures {
    NYT::TUserJobSpec* MakeCalculatorDataJobSpec(const TConfig& config, bool addKnnModel = false, const TVector<TString>& specificConfigFiles = {});

    TConfig* InitMapReduceConfig(const TString& configFileName, size_t version);
}
