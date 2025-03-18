#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/system/env.h>

#include <cstdlib>

using TVariablesMap = THashMap<TString, TString>;

inline TString TryResolveVariable(const TString &var, const TVariablesMap& variables) {
    if (!var.StartsWith('$')) {
        return var;
    }

    TString value;
    const auto& valueIt = variables.find(var.substr(1));
    if (valueIt != variables.end()) {
        value = valueIt->second;
    }
    else {
        TString env = GetEnv(var.substr(1).data());
        if (env) {
            value = env;
        }
    }
    return value;
}
