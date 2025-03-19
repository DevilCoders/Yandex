#include "weighted_percentile.h"

#include <util/generic/algorithm.h>
#include <util/system/yassert.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

const TVector<TPercentileDesc>& GetDefaultPercentiles()
{
    static const TVector<TPercentileDesc> DefaultPercentiles = {{
        { 000.5, "50"   },
        { 000.9, "90"   },
        { 00.99, "99"   },
        { 0.999, "99.9" },
        { 1.000, "100"  }
    }};

    return DefaultPercentiles;
}

const TVector<TString>& GetDefaultPercentileNames()
{
    static const TVector<TString> DefaultPercentiles = {{
        "50",
        "90",
        "99",
        "99.9",
        "100"
    }};

    return DefaultPercentiles;
}

////////////////////////////////////////////////////////////////////////////////

TVector<double> CalculateWeightedPercentiles(
    const TVector<TBucketInfo>& buckets,
    const TVector<TPercentileDesc>& percentiles)
{
    Y_VERIFY_DEBUG(IsSorted(buckets.begin(), buckets.end(),
        [] (const auto& l, const auto& r) { return l.first < r.first; }));

    TVector<double> result(Reserve(percentiles.size()));
    auto pit = percentiles.begin();
    auto bit = buckets.begin();

    ui64 prevSum = 0;
    ui64 prevLimit = 0;
    ui64 totalSum = Accumulate(
        buckets.begin(),
        buckets.end(),
        (ui64)0,
        [] (const auto& l, const auto& r) { return l + r.second; });

    while (bit != buckets.end() && pit != percentiles.end()) {
        double current = 0;
        if (totalSum) {
            current = (double)(prevSum + bit->second) / totalSum;
        }
        if (current >= pit->first) {
            auto delta = pit->first * totalSum - prevSum;
            auto part = (double)(bit->first - prevLimit)*delta;
            if (bit->second) {
                part /= bit->second;
            }
            auto point = prevLimit + part;
            result.push_back(point);
            ++pit;
        } else {
            prevSum += bit->second;
            prevLimit = bit->first;
            ++bit;
        }
    }

    if (pit != percentiles.end()) {
        auto last = result.size() ? result.back() : 0;
        for (; pit != percentiles.end(); ++pit) {
            result.push_back(last);
        }
    }

    return result;
}

}   // namespace NCloud
