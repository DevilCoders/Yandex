#include "time_normalizer.h"

#include <util/generic/vector.h>

#include <algorithm>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

TTimeNormalizer::TTimeNormalizer(
        TDuration windowTime,
        TDuration allowableFaultTime)
    : WindowTime(windowTime.MicroSeconds())
    , AllowableFaultTime(allowableFaultTime.MicroSeconds())
{}

void TTimeNormalizer::Init(const TVector<TString>& hosts)
{
    for (const auto& host: hosts) {
        HostsToTimeCache.try_emplace(host, TTimeCache{});
    }
}

void TTimeNormalizer::Normalize(
    const TString& host,
    THostMetrics& metrics)
{
    if (HostsToTimeCache[host].PreviousTime) {
        const i64 currentWindow =
            metrics.Timestamp.MicroSeconds() -
            HostsToTimeCache[host].PreviousTime +
            HostsToTimeCache[host].TimeOffset;
        const i64 faultTime = WindowTime - currentWindow;

        if (abs(faultTime) <= AllowableFaultTime) {
            HostsToTimeCache[host].TimeOffset += faultTime;
            if (HostsToTimeCache[host].TimeOffset >= 0) {
                metrics.Timestamp +=
                    TDuration::MicroSeconds(HostsToTimeCache[host].TimeOffset);
            } else {
                metrics.Timestamp -=
                    TDuration::MicroSeconds(-HostsToTimeCache[host].TimeOffset);
            }
        } else { // reboot host
            // to avoid calculate one host several times drop timestamp
            HostsToTimeCache[host].TimeOffset = 0;
        }
    }

    HostsToTimeCache[host].PreviousTime = metrics.Timestamp.MicroSeconds();
}

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
