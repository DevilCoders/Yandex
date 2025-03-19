#include "stats_service_state.h"

#include <cloud/blockstore/libs/common/proto_helpers.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

void TTotalCounters::Register(NMonitoring::TDynamicCountersPtr counters)
{
    PartAcc.Register(counters, true);
    VolumeAcc.Register(counters, true);
    TotalDiskCount.Register(counters, "TotalDiskCount");
    TotalPartitionCount.Register(counters, "TotalPartitionCount");
}

void TTotalCounters::Reset()
{
    PartAcc.Reset();
    VolumeAcc.Reset();
}

void TTotalCounters::Publish(TInstant now)
{
    PartAcc.Publish(now);
    VolumeAcc.Publish(now);
    TotalDiskCount.Publish(now);
    TotalPartitionCount.Publish(now);
}

void TTotalCounters::UpdatePartCounters(const TPartitionDiskCounters& source)
{
    PartAcc.AggregateWith(source);
}

void TTotalCounters::UpdateVolumeSelfCounters(const TVolumeSelfCounters& source)
{
    VolumeAcc.AggregateWith(source);
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeRequestCounters::Register(NMonitoring::TDynamicCountersPtr counters)
{
    ReadCount.Register(counters->GetSubgroup("request", "ReadBlocks"), "Count");
    ReadBytes.Register(counters->GetSubgroup("request", "ReadBlocks"), "RequestBytes");

    WriteCount.Register(counters->GetSubgroup("request", "WriteBlocks"), "Count");
    WriteBytes.Register(counters->GetSubgroup("request", "WriteBlocks"), "RequestBytes");

    ZeroCount.Register(counters->GetSubgroup("request", "ZeroBlocks"), "Count");
    ZeroBytes.Register(counters->GetSubgroup("request", "ZeroBlocks"), "RequestBytes");
}

void TVolumeRequestCounters::Publish(TInstant now)
{
    ReadCount.Publish(now);
    ReadBytes.Publish(now);

    WriteCount.Publish(now);
    WriteBytes.Publish(now);

    ZeroCount.Publish(now);
    ZeroBytes.Publish(now);

    Reset();
}

void TVolumeRequestCounters::Reset()
{
    ReadCount.Reset();
    ReadBytes.Reset();

    WriteCount.Reset();
    WriteBytes.Reset();

    ZeroCount.Reset();
    ZeroBytes.Reset();
}

void TVolumeRequestCounters::UpdateCounters(const TPartitionDiskCounters& source)
{
    ReadCount.Increment(source.RequestCounters.ReadBlocks.GetCount());
    ReadBytes.Increment(source.RequestCounters.ReadBlocks.GetRequestBytes());

    WriteCount.Increment(source.RequestCounters.WriteBlocks.GetCount());
    WriteBytes.Increment(source.RequestCounters.WriteBlocks.GetRequestBytes());

    ZeroCount.Increment(source.RequestCounters.ZeroBlocks.GetCount());
    ZeroBytes.Increment(source.RequestCounters.ZeroBlocks.GetRequestBytes());
}

////////////////////////////////////////////////////////////////////////////////

TBlobLoadCounters::TBlobLoadCounters(
        const TString& mediaKind,
        ui64 maxGroupReadIops,
        ui64 maxGroupWriteIops,
        ui64 maxGroupReadThroughput,
        ui64 maxGroupWriteThroughput)
    : MediaKind(mediaKind)
    , MaxGroupReadIops(maxGroupReadIops)
    , MaxGroupWriteIops(maxGroupWriteIops)
    , MaxGroupReadThroughput(maxGroupReadThroughput)
    , MaxGroupWriteThroughput(maxGroupWriteThroughput)
{}

void TBlobLoadCounters::Register(NMonitoring::TDynamicCountersPtr counters)
{
    UsedGroupsCount.Init(counters, "UsedGroupsHundredthsCount");
}

void TBlobLoadCounters::Publish(
    const NBlobMetrics::TBlobLoadMetrics& metrics,
    TInstant now)
{
    NBlobMetrics::TBlobLoadMetrics::TTabletMetric metricsAggregate;

    for (const auto& [kind, ops]: metrics.PoolKind2TabletOps) {
        if (kind.Contains(MediaKind)) {
            for (const auto& [key, metrics]: ops) {
                metricsAggregate += metrics;
            }
        }
    }

    const double groupReadIops =
        metricsAggregate.ReadOperations.Iops;
    const double groupWriteIops =
        metricsAggregate.WriteOperations.Iops;
    const double groupReadByteCounts =
        metricsAggregate.ReadOperations.ByteCount;
    const double groupWriteByteCounts =
        metricsAggregate.WriteOperations.ByteCount;

    const ui64 groupsCount = 100 *
        ( groupReadIops / MaxGroupReadIops +
          groupWriteIops / MaxGroupWriteIops +
          groupReadByteCounts / MaxGroupReadThroughput +
          groupWriteByteCounts / MaxGroupWriteThroughput ) /
        UpdateCountersInterval.SecondsFloat();

    UsedGroupsCount.SetCounterValue(now, groupsCount);
}

////////////////////////////////////////////////////////////////////////////////

void TStatsServiceState::RemoveVolume(const TString& diskId)
{
    VolumesById.erase(diskId);
}

TVolumeStatsInfo* TStatsServiceState::GetVolume(const TString& diskId)
{
    auto it = VolumesById.find(diskId);
    return it != VolumesById.end() ? &it->second : nullptr;
}

TVolumeStatsInfo* TStatsServiceState::GetOrAddVolume(const TString& diskId)
{
    TVolumesMap::insert_ctx ctx;
    auto it = VolumesById.find(diskId, ctx);
    if (it == VolumesById.end()) {
        it = VolumesById.emplace_direct(ctx, diskId, TVolumeStatsInfo());
    }

    return &it->second;
}

}   // namespace NCloud::NBlockStore::NStorage
