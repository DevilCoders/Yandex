#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NUtil {
    typedef TVector<std::pair<TString, TString>> TEnvVars;

    TEnvVars GetEnvironmentVariables();
}
