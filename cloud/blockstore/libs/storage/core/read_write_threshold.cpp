#include "read_write_threshold.h"

#include <cloud/blockstore/libs/diagnostics/config.h>

#include <cloud/storage/core/libs/diagnostics/weighted_percentile.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

constexpr ui32 Ms2UsFactor = 1000;

////////////////////////////////////////////////////////////////////////////////

TPerfThreshold GetThreshold(
    const TDiagnosticsConfig& config,
    bool isSsd)
{
    return isSsd ? config.GetSsdPerfThreshold() : config.GetHddPerfThreshold();
}

bool CheckPercentileLimit(
    ui64 threshold,
    double percentile,
    const NKikimr::TTabletPercentileCounter& small,
    const NKikimr::TTabletPercentileCounter& large)
{
    if (!threshold || !percentile) {
        return false;
    }
    NKikimr::TTabletPercentileCounter total;
    total.PopulateFrom(small);
    total.PopulateFrom(large);

    TVector<TBucketInfo> buckets(Reserve(total.GetRangeCount()));
    for (ui32 idxRange = 0; idxRange < total.GetRangeCount(); ++idxRange) {
        auto value = total.GetRangeValue(idxRange);
        buckets.emplace_back(
            total.GetRangeBound(idxRange),
            value);
    }

    auto result = CalculateWeightedPercentiles(
        buckets,
        { std::make_pair(percentile / 100, "")});

    return result[0] > (threshold * Ms2UsFactor);
}

}   // namespace NCloud::NBlockStore::NStorage
