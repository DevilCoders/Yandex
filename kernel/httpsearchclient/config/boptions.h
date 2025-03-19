#pragma once

#include <util/generic/string.h>

class TBalancingOptions {
public:
    size_t MaxAttempts;
    float BadDynWeightMult;
    float GoodDynWeightMult;
    float MinDynWeight;
    bool AllowDynamicWeights;
    bool RandomGroupSelection;
    float WeightDistributionThreshold;
    float RandomGroupSkipping;
    TString DynBalancerType;
    bool EnableInheritance;
    bool EnableIpV6;
    bool EnableUnresolvedHosts;
    size_t ParallelFetchRequestCount;
    bool PingZeroWeight;
    bool RawIpAddrs;
    bool AllowEmptySources;
    bool EnableCachedResolve;

public:
    inline TBalancingOptions() noexcept
        : MaxAttempts(2)
        , BadDynWeightMult(0.9f)
        , GoodDynWeightMult(1.2f)
        , MinDynWeight(0.01f)
        , AllowDynamicWeights(true)
        , RandomGroupSelection(false)
        , WeightDistributionThreshold(0.8f)
        , RandomGroupSkipping(0.0)
        , DynBalancerType("")
        , EnableInheritance(true)
        , EnableIpV6(true)
        , EnableUnresolvedHosts(false)
        , ParallelFetchRequestCount(1)
        , PingZeroWeight(true)
        , RawIpAddrs(false)
        , AllowEmptySources(false)
        , EnableCachedResolve(true)
    {
    }

    virtual ~TBalancingOptions() = default;

    void Parse(const TString& opts);

protected:
    virtual bool SetOption(const TString& name, const TString& value);
};
