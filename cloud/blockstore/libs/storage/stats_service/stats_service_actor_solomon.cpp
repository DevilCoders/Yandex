#include "stats_service_actor.h"

#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/storage/core/libs/diagnostics/histogram.h>
#include <cloud/storage/core/libs/diagnostics/weighted_percentile.h>

#include <ydb/core/base/appdata.h>

#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;
using namespace NMonitoring;
using namespace NProto;
using namespace NYdbStats;

namespace {

////////////////////////////////////////////////////////////////////////////////

void RegisterVolumeSelfCounters(
    std::shared_ptr<NUserCounter::TUserCounterSupplier> userCounters,
    NMonitoring::TDynamicCounterPtr& counters,
    TVolumeStatsInfo& volume)
{
    if (counters) {
        auto volumeCounters =
            counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "service_volume")
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", volume.VolumeInfo->GetDiskId());

        volume.PerfCounters.DiskCounters.Register(volumeCounters, false);
        volume.PerfCounters.VolumeSelfCounters.Register(volumeCounters, false);
        volume.PerfCounters.VolumeBindingCounter = volumeCounters->GetCounter("LocalVolume", false);
        volume.PerfCounters.CountersRegistered = true;

        NUserCounter::RegisterServiceVolume(
            *userCounters,
            volume.VolumeInfo->GetCloudId(),
            volume.VolumeInfo->GetFolderId(),
            volume.VolumeInfo->GetDiskId(),
            volumeCounters);
    }
}

void UnregisterVolumeSelfCounters(
    std::shared_ptr<NUserCounter::TUserCounterSupplier> userCounters,
    NMonitoring::TDynamicCounterPtr& counters,
    const TString& diskId,
    TVolumeStatsInfo& volume)
{
    if (counters) {
        counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "service_volume")
            ->GetSubgroup("host", "cluster")
            ->RemoveSubgroup("volume", diskId);
    }

    volume.PerfCounters.CountersRegistered = false;

    NUserCounter::UnregisterServiceVolume(
        *userCounters,
        volume.VolumeInfo->GetCloudId(),
        volume.VolumeInfo->GetFolderId(),
        volume.VolumeInfo->GetDiskId());
}

}    // namespace

////////////////////////////////////////////////////////////////////////////////

void TStatsServiceActor::UpdateVolumeSelfCounters(const TActorContext& ctx)
{
    TVector<TTotalCounters*> totalCounters{
        &State.GetTotalCounters(),
        &State.GetSsdCounters(),
        &State.GetHddCounters(),
        &State.GetSsdNonreplCounters(),
        &State.GetSsdSystemCounters(),
    };

    for (auto* tc: totalCounters) {
        tc->TotalDiskCount.Reset();
        tc->TotalPartitionCount.Reset();
    }

    NBlobMetrics::TBlobLoadMetrics tempBlobMetrics;

    for (auto& p: State.GetVolumes()) {
        auto& vol = p.second;
        if (!vol.VolumeInfo.Defined()) {
            continue;
        }
        if (vol.PerfCounters.HasCheckpoint || vol.PerfCounters.HasClients) {
            if (!vol.PerfCounters.CountersRegistered) {
                RegisterVolumeSelfCounters(
                    UserCounters,
                    AppData(ctx)->Counters,
                    vol);
            }

            Y_VERIFY(vol.PerfCounters.CountersRegistered);
            vol.PerfCounters.DiskCounters.Publish(ctx.Now());
            vol.PerfCounters.VolumeSelfCounters.Publish(ctx.Now());
            *vol.PerfCounters.VolumeBindingCounter = !vol.PerfCounters.IsPreempted;
        } else if (vol.PerfCounters.CountersRegistered) {
            UnregisterVolumeSelfCounters(
                UserCounters,
                AppData(ctx)->Counters,
                p.first,
                vol);
        }

        auto& tc = State.GetCounters(*vol.VolumeInfo);
        tc.TotalDiskCount.Increment(1);
        tc.TotalPartitionCount.Increment(vol.VolumeInfo->GetPartitionsCount());

        auto& serviceTotal = State.GetTotalCounters();
        serviceTotal.TotalDiskCount.Increment(1);
        serviceTotal.TotalPartitionCount.Increment(vol.VolumeInfo->GetPartitionsCount());
        tempBlobMetrics += vol.OffsetBlobMetrics;
    }

    for (auto* tc: totalCounters) {
        tc->Publish(ctx.Now());
    }

    State.GetSsdBlobCounters().Publish(tempBlobMetrics, ctx.Now());
    State.GetHddBlobCounters().Publish(tempBlobMetrics, ctx.Now());

    CurrentBlobMetrics += tempBlobMetrics;

    State.GetLocalVolumesCounters().Publish(ctx.Now());
    State.GetNonlocalVolumesCounters().Publish(ctx.Now());
}

void TStatsServiceActor::HandleRegisterVolume(
    const TEvStatsService::TEvRegisterVolume::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);
    const auto* msg = ev->Get();

    auto volume = State.GetOrAddVolume(msg->DiskId);
    volume->VolumeInfo = msg->Config;

    if (volume->IsNonReplicated()) {
        volume->PerfCounters = TDiskPerfData(EPublishingPolicy::NonRepl);
    } else {
        volume->PerfCounters = TDiskPerfData(EPublishingPolicy::Repl);
    }
}

void TStatsServiceActor::HandleVolumeConfigUpdated(
    const TEvStatsService::TEvVolumeConfigUpdated::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto volume = State.GetVolume(msg->DiskId);
    if (!volume) {
        LOG_WARN(ctx, TBlockStoreComponents::STATS_SERVICE,
            "Volume %s not found",
            msg->DiskId.Quote().data());
        return;
    }
    volume->VolumeInfo = msg->Config;
}

void TStatsServiceActor::HandleUnregisterVolume(
    const TEvStatsService::TEvUnregisterVolume::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto& volumes = State.GetVolumes();

    if (auto it = volumes.find(msg->DiskId); it != volumes.end()) {
        UnregisterVolumeSelfCounters(
            UserCounters,
            AppData(ctx)->Counters,
            it->first,
            it->second);

        State.RemoveVolume(it->first);
    }
}

void TStatsServiceActor::HandleVolumePartCounters(
    const TEvStatsService::TEvVolumePartCounters::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto volume = State.GetVolume(msg->DiskId);

    if (!volume) {
        LOG_DEBUG(ctx, TBlockStoreComponents::STATS_SERVICE,
            "Volume %s for counters not found",
            msg->DiskId.Quote().data());
        return;
    }

    if (!volume->VolumeInfo.Defined()) {
        return;
    }

    volume->PerfCounters.VolumeSystemCpu += msg->VolumeSystemCpu;
    volume->PerfCounters.VolumeUserCpu += msg->VolumeUserCpu;
    volume->PerfCounters.HasCheckpoint = msg->HasCheckpoint;

    volume->PerfCounters.DiskCounters.Add(*msg->DiskCounters);
    volume->PerfCounters.YdbDiskCounters.Add(*msg->DiskCounters);
    volume->OffsetBlobMetrics = msg->BlobLoadMetrics;

    State.GetTotalCounters().UpdatePartCounters(*msg->DiskCounters);

    State.GetCounters(*volume->VolumeInfo).UpdatePartCounters(*msg->DiskCounters);

    if (ev->Sender.NodeId() == SelfId().NodeId()) {
        State.GetLocalVolumesCounters().UpdateCounters(*msg->DiskCounters);
    } else {
        State.GetNonlocalVolumesCounters().UpdateCounters(*msg->DiskCounters);
    }
}

void TStatsServiceActor::HandleVolumeSelfCounters(
    const TEvStatsService::TEvVolumeSelfCounters::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto volume = State.GetVolume(msg->DiskId);

    if (!volume || !volume->VolumeInfo.Defined()) {
        LOG_DEBUG(ctx, TBlockStoreComponents::STATS_SERVICE,
            "Volume %s for counters not found",
            msg->DiskId.Quote().data());
        return;
    }

    volume->PerfCounters.VolumeSelfCounters.Add(*msg->VolumeSelfCounters);
    volume->PerfCounters.YdbVolumeSelfCounters.Add(*msg->VolumeSelfCounters);
    volume->PerfCounters.HasClients = msg->HasClients;
    volume->PerfCounters.IsPreempted = msg->IsPreempted;

    State.GetTotalCounters().UpdateVolumeSelfCounters(*msg->VolumeSelfCounters);

    State.GetCounters(*volume->VolumeInfo).UpdateVolumeSelfCounters(*msg->VolumeSelfCounters);
}

}   // namespace NCloud::NBlockStore::NStorage
