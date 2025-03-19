#include "boptions.h"

#include <util/string/cast.h>
#include <util/string/type.h>
#include <util/generic/yexception.h>

void TBalancingOptions::Parse(const TString& opts_in) {
    TString opts = opts_in + ',';
    TString key;
    TString val;
    TString* cur = &key;
    TString cacheLifeTimeOpts;
    bool rawInputMode = false;

    for (TString::const_iterator it = opts.begin(); it != opts.end(); ++it) {
        const char ch = *it;

        if (ch == '\"') {
            rawInputMode = !rawInputMode;
        } else if (!rawInputMode && (ch == ' ' || ch == '\t')) {

        } else if (!rawInputMode && (ch == '=')) {
            cur = &val;
        } else if (!rawInputMode && (ch == ',')) {
            cur = &key;

            if (key.empty() && val.empty()) {
                continue;
            }

            try {
                if (!SetOption(key, val)) {
                    Cerr << "can not parse " << key << " -> " << val << Endl;
                }
            } catch (...) {
                Cerr << CurrentExceptionMessage() << Endl;
            }

            key.clear();
            val.clear();
        } else {
            *cur += ch;
        }
    }
}

bool TBalancingOptions::SetOption(const TString& name, const TString& value) {
    if (name == "MaxAttempts") {
        MaxAttempts = FromString(value);
    } else if (name == "BadDynWeightMult") {
        BadDynWeightMult = FromString(value);
    } else if (name == "GoodDynWeightMult") {
        GoodDynWeightMult = FromString(value);
    } else if (name == "MinDynWeight") {
        MinDynWeight = FromString(value);
    } else if (name == "AllowDynamicWeights") {
        AllowDynamicWeights = IsTrue(value);
    } else if (name == "RandomGroupSelection") {
        RandomGroupSelection = IsTrue(value);
    } else if (name == "WeightDistributionThreshold") {
        WeightDistributionThreshold = FromString(value);
    } else if (name == "RandomGroupSkipping") {
        RandomGroupSkipping = FromString(value);
    } else if (name == "DynBalancerType") {
        DynBalancerType = value;
    } else if (name == "EnableInheritance") {
        EnableInheritance = IsTrue(value);
    } else if (name == "EnableIpV6") {
        EnableIpV6 = IsTrue(value);
    } else if (name == "EnableUnresolvedHosts") {
        EnableUnresolvedHosts = IsTrue(value);
    } else if (name == "ParallelFetchRequestCount") {
        ParallelFetchRequestCount = FromString<size_t>(value);
    } else if (name == "PingZeroWeight") {
        PingZeroWeight = IsTrue(value);
    } else if (name == "RawIpAddrs") {
        RawIpAddrs = IsTrue(value);
    } else if (name == "AllowEmptySources") {
        AllowEmptySources = IsTrue(value);
    } else if (name == "EnableCachedResolve") {
        EnableCachedResolve = IsTrue(value);
    } else {
        return false;
    }
    return true;
}
