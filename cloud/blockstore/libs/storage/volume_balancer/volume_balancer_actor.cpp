#include "volume_balancer_actor.h"

#include "volume_balancer_events_private.h"

#include <cloud/blockstore/libs/diagnostics/cgroup_stats_fetcher.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_balancer.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <ydb/core/base/appdata.h>
#include <ydb/core/mon/mon.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NMetrics;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration Timeout = TDuration::Seconds(15);
constexpr ui32 CpuLackPercentsMultiplier = 100;

////////////////////////////////////////////////////////////////////////////////

class TRemoteVolumeStatActor final
    : public TActorBootstrapped<TRemoteVolumeStatActor>
{
private:
    const TActorId VolumeBalancerActorId;
    const TVector<TString> RemoteVolumes;
    const TDuration Timeout;

    ui32 Responses = 0;

    TVector<NProto::TVolumeBalancerDiskStats> Stats;

public:
    TRemoteVolumeStatActor(
            TActorId volumeBalancerActorId,
            TVector<TString> remoteVolumes,
            TDuration timeout)
        : VolumeBalancerActorId(volumeBalancerActorId)
        , RemoteVolumes(std::move(remoteVolumes))
        , Timeout(timeout)
    {}

    void Bootstrap(const TActorContext& ctx)
    {
        Y_UNUSED(ctx);

        for (const auto& v: RemoteVolumes) {
            auto request = std::make_unique<TEvVolume::TEvGetVolumeLoadInfoRequest>();
            request->Record.SetDiskId(v);
        }
        Become(&TThis::StateWork);

        ctx.Schedule(
            Timeout,
            new TEvents::TEvWakeup()
        );

    }

private:
    STFUNC(StateWork);

    void HandleGetVolumeLoadInfoResponse(
        const TEvVolume::TEvGetVolumeLoadInfoResponse::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto* msg = ev->Get();

        if (FAILED(msg->GetStatus())) {
            LOG_ERROR(ctx, TBlockStoreComponents::VOLUME_BALANCER,
                "Failed to get stats for remote volume %s",
                msg->Record.GetStats().GetDiskId().c_str());
        } else {
            LOG_INFO(ctx, TBlockStoreComponents::VOLUME_BALANCER,
                "Got stats for remote volume %s",
                msg->Record.GetStats().GetDiskId().c_str());
            Stats.push_back(msg->Record.GetStats());
        }
        if (++Responses == RemoteVolumes.size()) {
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
        auto response = std::make_unique<TEvVolumeBalancerPrivate::TEvRemoteVolumeStats>();
        response->Stats = std::move(Stats);
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
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME_BALANCER);
            break;
    }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

TVolumeBalancerActor::TVolumeBalancerActor(
        NKikimrConfig::TAppConfigPtr appConfig,
        TStorageConfigPtr storageConfig,
        IVolumeStatsPtr volumeStats,
        ICgroupStatsFetcherPtr cgroupStatsFetcher,
        TActorId serviceActorId)
    : StorageConfig(std::move(storageConfig))
    , VolumeStats(std::move(volumeStats))
    , CgroupStatsFetcher(std::move(cgroupStatsFetcher))
    , ServiceActorId(serviceActorId)
    , State(std::make_unique<TVolumeBalancerState>(StorageConfig))
{
    if (appConfig->GetActorSystemConfig().HasSysExecutor()) {
        ui32 idx = appConfig->GetActorSystemConfig().GetSysExecutor();
        SystemThreadCount = appConfig->GetActorSystemConfig().GetExecutor(idx).GetThreads();
    }

    if (appConfig->GetActorSystemConfig().HasUserExecutor()) {
        ui32 idx = appConfig->GetActorSystemConfig().GetUserExecutor();
        UserThreadCount = appConfig->GetActorSystemConfig().GetExecutor(idx).GetThreads();
    }
}

void TVolumeBalancerActor::Bootstrap(const TActorContext& ctx)
{
    RegisterPages(ctx);
    RegisterCounters(ctx);
    Become(&TThis::StateWork);
}

void TVolumeBalancerActor::RegisterPages(const TActorContext& ctx)
{
    auto mon = AppData(ctx)->Mon;
    if (mon) {
        auto* rootPage = mon->RegisterIndexPage("blockstore", "BlockStore");

        mon->RegisterActorPage(rootPage, "balancer", "Balancer",
            false, ctx.ExecutorThread.ActorSystem, SelfId());
    }
}

void TVolumeBalancerActor::RegisterCounters(const TActorContext& ctx)
{
    auto counters = AppData(ctx)->Counters;
    auto rootGroup = counters->GetSubgroup("counters", "blockstore");
    auto serviceCounters = rootGroup->GetSubgroup("component", "service");
    auto serverCounters = rootGroup->GetSubgroup("component", "server");
    PushCount = serviceCounters->GetCounter("PushCount", true);
    PullCount = serviceCounters->GetCounter("PullCount", true);

    CpuWait = serverCounters->GetCounter("CpuWait", false);

    SysCPULoad =
        counters->GetSubgroup("counters", "utils")
        ->GetSubgroup("execpool", "System")
        ->GetCounter("ElapsedMicrosec", true);

    UserCPULoad =
        counters->GetSubgroup("counters", "utils")
        ->GetSubgroup("execpool", "User")
        ->GetCounter("ElapsedMicrosec", true);

    CpuMatBenchSys =
        counters->GetSubgroup("counters", "utils")
        ->GetSubgroup("execpool", "System")
        ->GetCounter("CpuMatBenchNs", false);

    CpuMatBenchUser =
        counters->GetSubgroup("counters", "utils")
        ->GetSubgroup("execpool", "User")
        ->GetCounter("CpuMatBenchNs", false);

    ctx.Schedule(Timeout, new TEvents::TEvWakeup);
}

bool TVolumeBalancerActor::IsBalancerEnabled() const
{
    return State->GetEnabled()
        && StorageConfig->GetVolumePreemptionType() != NProto::PREEMPTION_NONE;
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeBalancerActor::PullVolumeFromHive(
    const TActorContext& ctx,
    const TString& volume)
{
    LOG_INFO(ctx, TBlockStoreComponents::VOLUME_BALANCER,
        "Pull volume %s from hive",
        volume.data());

    auto pullRequest = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
        volume,
        TEvService::TEvChangeVolumeBindingRequest::EChangeBindingOp::ACQUIRE_FROM_HIVE,
        NProto::EPreemptionSource::SOURCE_BALANCER);
    NCloud::Send(ctx, ServiceActorId, std::move(pullRequest));

    PullCount->Add(1);
}

void TVolumeBalancerActor::SendVolumeToHive(
    const TActorContext& ctx,
    const TString& volume)
{
    LOG_INFO(ctx, TBlockStoreComponents::VOLUME_BALANCER,
        "Push volume %s to hive",
        volume.data());

    auto pushRequest = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
        volume,
        TEvService::TEvChangeVolumeBindingRequest::EChangeBindingOp::RELEASE_TO_HIVE,
        NProto::EPreemptionSource::SOURCE_BALANCER);
    NCloud::Send(ctx, ServiceActorId, std::move(pushRequest));

    PushCount->Add(1);
}

void TVolumeBalancerActor::HandleGetVolumeStatsResponse(
    const TEvService::TEvGetVolumeStatsResponse::TPtr& ev,
    const TActorContext& ctx)
{
    if (State) {
        const auto *msg = ev->Get();

        auto now = ctx.Now();

        double newSysCpuLoad = (*SysCPULoad - PrevSysCPULoad) / Timeout.Seconds();
        double newUserCpuLoad = (*UserCPULoad - PrevUserCPULoad) / Timeout.Seconds();
        PrevSysCPULoad = *SysCPULoad;
        PrevUserCPULoad = *UserCPULoad;

        double sysPercentage = 100 * newSysCpuLoad / (SystemThreadCount * DurationPerSecond);
        double userPercentage = 100 * newUserCpuLoad / (UserThreadCount * DurationPerSecond);

        auto interval = (now - LastCpuWaitQuery).MicroSeconds();
        auto cpuLack = CpuLackPercentsMultiplier * CgroupStatsFetcher->GetCpuWait().MicroSeconds();
        cpuLack /= interval;
        *CpuWait = cpuLack;

        LastCpuWaitQuery = now;

        LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME_BALANCER,
            "Got GetVolumeStatsResponse (%f, %f) (%f, %f) %lu %x",
            newSysCpuLoad,
            newUserCpuLoad,
            sysPercentage,
            userPercentage,
            cpuLack,
            State->GetState());

        auto status = VolumeStats->GatherVolumePerfStatuses();
        THashMap<TString, bool> statusMap(status.begin(), status.end());

        State->UpdateVolumeStats(
            std::move(msg->VolumeStats),
            std::move(statusMap),
            sysPercentage,
            userPercentage,
            *CpuMatBenchSys,
            *CpuMatBenchUser,
            cpuLack,
            now);

        if (IsBalancerEnabled()) {
            auto vol = State->GetVolumeToPush();

            if (vol) {
                SendVolumeToHive(ctx, vol);
            } else {
                vol = State->GetVolumeToPull();
                if (vol) {
                    PullVolumeFromHive(ctx, vol);
                }
            }
        }

        ctx.Schedule(Timeout, new TEvents::TEvWakeup());
    }
}

void TVolumeBalancerActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    auto msg = std::make_unique<TEvService::TEvGetVolumeStatsRequest>();
    NCloud::Send(ctx, ServiceActorId, std::move(msg));
}

void TVolumeBalancerActor::HandleBindingResponse(
    const TEvService::TEvChangeVolumeBindingResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (HasError(msg->GetError())) {
        LOG_ERROR(ctx, TBlockStoreComponents::VOLUME_BALANCER,
            "Failed to change volume binding %s",
            msg->DiskId.Quote().data());
    }
}

void TVolumeBalancerActor::HandleConfigureVolumeBalancerRequest(
    const TEvVolumeBalancer::TEvConfigureVolumeBalancerRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto response = std::make_unique<TEvVolumeBalancer::TEvConfigureVolumeBalancerResponse>();
    if (State->GetEnabled()) {
        response->Record.SetOpStatus(NPrivateProto::EBalancerOpStatus::ENABLE);
    } else {
        response->Record.SetOpStatus(NPrivateProto::EBalancerOpStatus::DISABLE);
    }

    NCloud::Send(ctx, ev->Sender, std::move(response));

    const auto* msg = ev->Get();

    switch (msg->Record.GetOpStatus()) {
        case NPrivateProto::EBalancerOpStatus::ENABLE: {
            State->SetEnabled(true);
            break;
        }
        case NPrivateProto::EBalancerOpStatus::DISABLE: {
            State->SetEnabled(false);
            break;
        }
        default: {
            // remain the same
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TVolumeBalancerActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        HFunc(TEvService::TEvChangeVolumeBindingResponse, HandleBindingResponse);
        HFunc(TEvService::TEvGetVolumeStatsResponse, HandleGetVolumeStatsResponse);
        HFunc(TEvVolumeBalancer::TEvConfigureVolumeBalancerRequest, HandleConfigureVolumeBalancerRequest);

        HFunc(NMon::TEvHttpInfo, HandleHttpInfo);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME_BALANCER);
            break;
    }
}

}   // namespace NCloud::NBlockStore::NStorage
