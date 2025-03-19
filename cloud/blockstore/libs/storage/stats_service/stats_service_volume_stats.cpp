#include "stats_service_actor.h"

#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>

#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;
using namespace NMonitoring;
using namespace NProto;
using namespace NYdbStats;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration Timeout = TDuration::Seconds(5);

////////////////////////////////////////////////////////////////////////////////

class TRemoteVolumeStatActor final
    : public TActorBootstrapped<TRemoteVolumeStatActor>
{
private:
    const TActorId VolumeBalancerActorId;
    TVector<NProto::TVolumeBalancerDiskStats> Volumes;
    const TDuration Timeout;

    ui32 ResponsesToGet = 0;

    THashMap<TString, ui32> DiskToIdx;

public:
    TRemoteVolumeStatActor(
            TActorId volumeBalancerActorId,
            TVector<NProto::TVolumeBalancerDiskStats> volumes,
            TDuration timeout)
        : VolumeBalancerActorId(volumeBalancerActorId)
        , Volumes(std::move(volumes))
        , Timeout(timeout)
    {}

    void Bootstrap(const TActorContext& ctx)
    {
        for (ui32 i = 0; i < Volumes.size(); ++i) {
            const auto& v = Volumes[i];
            if (!v.GetHost()) {
                auto request = std::make_unique<TEvVolume::TEvGetVolumeLoadInfoRequest>();
                request->Record.SetDiskId(v.GetDiskId());
                NCloud::Send(ctx, MakeVolumeProxyServiceId(), std::move(request), 0);
                ++ResponsesToGet;
                DiskToIdx[v.GetDiskId()] = i;
            }
        }

        if (!ResponsesToGet) {
            ReplyAndDie(ctx);
            return;
        }

        ctx.Schedule(
            Timeout,
            new TEvents::TEvWakeup()
        );

        Become(&TThis::StateWork);
    }

private:
    STFUNC(StateWork);

    void HandleGetVolumeLoadInfoResponse(
        const TEvVolume::TEvGetVolumeLoadInfoResponse::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto* msg = ev->Get();

        if (FAILED(msg->GetStatus())) {
            LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
                "Failed to get stats for remote volume %s",
                msg->Record.GetStats().GetDiskId().c_str());
        } else {
            LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
                "Got stats for remote volume %s",
                msg->Record.GetStats().GetDiskId().c_str());

            const auto& diskId = msg->Record.GetStats().GetDiskId();
            auto it = DiskToIdx.find(diskId);
            Y_VERIFY(it != DiskToIdx.end());
            Volumes[it->second] = msg->Record.GetStats();
        }
        if (!--ResponsesToGet) {
            ReplyAndDie(ctx);
        }
    }

    void HandleWakeup(
        const TEvents::TEvWakeup::TPtr& ev,
        const TActorContext& ctx)
    {
        Y_UNUSED(ev);
        ReplyAndDie(ctx);
    }

    void ReplyAndDie(const TActorContext& ctx)
    {
        TVector<NProto::TVolumeBalancerDiskStats> collectedVolumes;

        for (auto& d : Volumes) {
            collectedVolumes.push_back(std::move(d));
        }

        auto response = std::make_unique<TEvStatsService::TEvGetVolumeStatsResponse>(
            std::move(collectedVolumes));

        NCloud::Send(ctx, VolumeBalancerActorId, std::move(response));

        Die(ctx);
    }
};

////////////////////////////////////////////////////////////////////////////////


STFUNC(TRemoteVolumeStatActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvVolume::TEvGetVolumeLoadInfoResponse, HandleGetVolumeLoadInfoResponse);
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::STATS_SERVICE);
            break;
    }
}

}    // namespace

////////////////////////////////////////////////////////////////////////////////

void TStatsServiceActor::HandleGetVolumeStats(
    const TEvStatsService::TEvGetVolumeStatsRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto& volumes = State.GetVolumes();

    for (auto& v: ev->Get()->VolumeStats) {
        if (v.GetHost()) {
            if (auto it = volumes.find(v.GetDiskId()); it != volumes.end()) {
                v.SetSystemCpu(it->second.PerfCounters.VolumeSystemCpu);
                v.SetUserCpu(it->second.PerfCounters.VolumeUserCpu);
                it->second.PerfCounters.VolumeSystemCpu = 0;
                it->second.PerfCounters.VolumeUserCpu = 0;
            }
        }
    }
    NCloud::Register<TRemoteVolumeStatActor>(
        ctx,
        ev->Sender,
        std::move(ev->Get()->VolumeStats),
        Timeout);
}

}   // namespace NCloud::NBlockStore::NStorage
