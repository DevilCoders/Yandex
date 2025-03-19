#pragma once

#include <kernel/facts/factors_info/factor_names.h>

#include <util/generic/map.h>
#include <util/generic/string.h>

namespace NFactClassifiers {

    using TQueryFactorStorageCache = TMap<TString, NUnstructuredFeatures::TFactFactorStorage>;

};
