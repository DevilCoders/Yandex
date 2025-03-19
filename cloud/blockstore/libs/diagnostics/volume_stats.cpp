#include "volume_stats.h"

#include "config.h"
#include "stats_helpers.h"
#include "user_counter.h"
#include "volume_perf.h"

#include <cloud/blockstore/libs/service/request_helpers.h>

#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/datetime/cputimer.h>
#include <util/generic/hash.h>
#include <util/system/rwlock.h>

#include <unordered_map>

namespace NCloud::NBlockStore {

using namespace NMonitoring;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TBusyIdleTimeCalculator
{
    enum EState
    {
        IDLE,
        BUSY,
        MAX
    };

    struct TStateInfo
    {
        TAtomic StartTime = 0;
        TIntrusivePtr<TCounterForPtr> Counter;
    };

    std::array<TStateInfo, EState::MAX> State;
    TAtomic InflightCount = 0;

public:
    TBusyIdleTimeCalculator()
    {
        StartState(IDLE);
    }

    void Register(TDynamicCounters& counters)
    {
        State[IDLE].Counter = counters.GetCounter("IdleTime", true);
        State[BUSY].Counter = counters.GetCounter("BusyTime", true);
    }

    void OnRequestStarted()
    {
        if (AtomicIncrement(InflightCount) == 1) {
            FinishState(IDLE);
            StartState(BUSY);
        }
    }

    void OnRequestCompleted()
    {
        if (AtomicDecrement(InflightCount) == 0) {
            FinishState(BUSY);
            StartState(IDLE);
        }
    }

    void OnUpdateStats()
    {
        UpdateProgress(IDLE);
        UpdateProgress(BUSY);
    }

private:
    void FinishState(EState state)
    {
        ui64 val = MicroSeconds() - AtomicSwap(&State[state].StartTime, 0);
        *State[state].Counter += val;
    }

    void StartState(EState state)
    {
        AtomicSet(State[state].StartTime, MicroSeconds());
    }

    void UpdateProgress(EState state)
    {
        for (;;) {
            ui64 started = AtomicGet(State[state].StartTime);
            if (!started) {
                return;
            }

            auto now = MicroSeconds();
            if (AtomicCas(&State[state].StartTime, now, started)) {
                *State[state].Counter += now - started;
                return;
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TVolumeInfoBase
{
    const NProto::TVolume Volume;
    TBusyIdleTimeCalculator BusyIdleCalc;
    TVolumePerformanceCalculator PerfCalc;

    TVolumeInfoBase(
            NProto::TVolume volume,
            TDiagnosticsConfigPtr diagnosticsConfig)
        : Volume(std::move(volume))
        , PerfCalc(Volume, std::move(diagnosticsConfig))
    {}
};

////////////////////////////////////////////////////////////////////////////////

class TVolumeInfo final
    : public IVolumeInfo
{
    friend class TVolumeStats;

private:
    const std::shared_ptr<TVolumeInfoBase> VolumeBase;

    const TString ClientId;
    const TString InstanceId;

    TRequestCounters RequestCounters;

    TDuration InactivityTimeout;
    TInstant LastRemountTime;

public:
    TVolumeInfo(
            std::shared_ptr<TVolumeInfoBase> volumeBase,
            ITimerPtr timer,
            TString clientId,
            TString instanceId)
        : VolumeBase(std::move(volumeBase))
        , ClientId(std::move(clientId))
        , InstanceId(std::move(instanceId))
        , RequestCounters(MakeRequestCounters(
            std::move(timer),
            TRequestCounters::EOption::OnlyReadWriteRequests))
    {}

    const NProto::TVolume& GetInfo() const override
    {
        return VolumeBase->Volume;
    }

    ui64 RequestStarted(
        EBlockStoreRequest requestType,
        ui32 requestBytes) override
    {
        VolumeBase->BusyIdleCalc.OnRequestStarted();
        return RequestCounters.RequestStarted(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)),
            requestBytes);
    }

    TDuration RequestCompleted(
        EBlockStoreRequest requestType,
        ui64 requestStarted,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind) override
    {
        VolumeBase->BusyIdleCalc.OnRequestCompleted();
        VolumeBase->PerfCalc.OnRequestCompleted(
            TranslateLocalRequestType(requestType),
            requestStarted,
            GetCycleCount(),
            DurationToCyclesSafe(postponedTime),
            requestBytes);
        return RequestCounters.RequestCompleted(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)),
            requestStarted,
            postponedTime,
            requestBytes,
            errorKind);
    }

    void AddIncompleteStats(
        EBlockStoreRequest requestType,
        TDuration requestTime) override
    {
        RequestCounters.AddIncompleteStats(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)),
            requestTime);
    }

    void AddRetryStats(
        EBlockStoreRequest requestType,
        EErrorKind errorKind) override
    {
        RequestCounters.AddRetryStats(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)),
            errorKind);
    }

    void RequestPostponed(EBlockStoreRequest requestType) override
    {
        RequestCounters.RequestPostponed(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)));
    }

    void RequestAdvanced(EBlockStoreRequest requestType) override
    {
        RequestCounters.RequestAdvanced(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)));
    }

    void RequestFastPathHit(EBlockStoreRequest requestType) override
    {
        RequestCounters.RequestFastPathHit(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TVolumeStats final
    : public IVolumeStats
{
    using TVolumeBasePtr = std::shared_ptr<TVolumeInfoBase>;
    using TVolumeInfoPtr = std::shared_ptr<TVolumeInfo>;
    using TVolumeMap = std::unordered_map<TString, TVolumeInfoPtr>;

    struct TVolumeInfoHolder
    {
        TVolumeBasePtr VolumeBase;
        TVolumeMap VolumeInfos;
    };

    using TVolumeHolderMap = std::unordered_map<TString, TVolumeInfoHolder>;

private:
    const IMonitoringServicePtr Monitoring;
    const TDuration InactiveClientsTimeout;
    const TDiagnosticsConfigPtr DiagnosticsConfig;
    const EVolumeStatsType Type;
    const ITimerPtr Timer;

    TDynamicCountersPtr Counters;
    std::shared_ptr<NUserCounter::TUserCounterSupplier> UserCounters;
    std::unique_ptr<TSufferCounters> SufferCounters;

    TVolumeHolderMap Volumes;
    TRWMutex Lock;

public:
    TVolumeStats(
            IMonitoringServicePtr monitoring,
            TDuration inactiveClientsTimeout,
            TDiagnosticsConfigPtr diagnosticsConfig,
            EVolumeStatsType type,
            ITimerPtr timer)
        : Monitoring(std::move(monitoring))
        , InactiveClientsTimeout(inactiveClientsTimeout)
        , DiagnosticsConfig(std::move(diagnosticsConfig))
        , Type(type)
        , Timer(std::move(timer))
        , UserCounters(std::make_shared<NUserCounter::TUserCounterSupplier>())
    {
    }

    bool MountVolume(
        const NProto::TVolume& volume,
        const TString& clientId,
        const TString& instanceId) override
    {
        TWriteGuard guard(Lock);

        bool inserted = false;

        auto volumeIt = Volumes.find(volume.GetDiskId());
        if (volumeIt == Volumes.end()) {
            volumeIt = Volumes.emplace(
                volume.GetDiskId(),
                RegisterVolume(volume)).first;
        }

        auto clientIt = volumeIt->second.VolumeInfos.find(clientId);
        if (clientIt == volumeIt->second.VolumeInfos.end()) {
            clientIt = volumeIt->second.VolumeInfos.emplace(
                clientId,
                RegisterClient(
                    volumeIt->second.VolumeBase,
                    clientId,
                    instanceId)).first;
            inserted = true;
        }

        clientIt->second->LastRemountTime = Timer->Now();
        clientIt->second->InactivityTimeout = InactiveClientsTimeout;

        return inserted;
    }

    void UnmountVolume(
        const TString& diskId,
        const TString& clientId) override
    {
        Y_UNUSED(clientId);
        Y_UNUSED(diskId);
    }

    IVolumeInfoPtr GetVolumeInfo(
        const TString& diskId,
        const TString& clientId) const override
    {
        TReadGuard guard(Lock);

        const auto volumeIt = Volumes.find(diskId);
        if (volumeIt != Volumes.end()) {
            const auto clientIt = volumeIt->second.VolumeInfos.find(clientId);
            return clientIt != volumeIt->second.VolumeInfos.end()
                ? clientIt->second
                : nullptr;
        }
        return nullptr;
    }

    ui32 GetBlockSize(const TString& diskId) const override
    {
        TWriteGuard guard(Lock);

        const auto volumeIt = Volumes.find(diskId);
        return volumeIt != Volumes.end()
            ? volumeIt->second.VolumeBase->Volume.GetBlockSize()
            : DefaultBlockSize;
    }

    bool TrimClients(const TInstant& now, TVolumeMap& infos)
    {
        std::erase_if(infos, [this, &now](const auto& item){
            const auto& [key, info] = item;
            if (info->InactivityTimeout &&
                now - info->LastRemountTime > info->InactivityTimeout)
            {
                UnregisterClient(
                    info->VolumeBase,
                    info->ClientId,
                    info->InstanceId);
                return true;
            }
            return false;
        });
        return infos.empty();
    }

    void TrimVolumes() override
    {
        TWriteGuard guard(Lock);

        const auto now = Timer->Now();

        std::erase_if(Volumes, [this, &now](auto& item) {
            auto& [key, holder] = item;
            if (TrimClients(now, holder.VolumeInfos)) {
                UnregisterVolume(holder.VolumeBase);
                return true;
            }
            return false;
        });
    }

    void UpdateStats(bool updatePercentiles) override
    {
        TReadGuard guard(Lock);

        for (auto& [volumeKey, volume]: Volumes) {
            volume.VolumeBase->BusyIdleCalc.OnUpdateStats();
            volume.VolumeBase->PerfCalc.UpdateStats();
            for (auto& [clientKey, client]: volume.VolumeInfos) {
                client->RequestCounters.UpdateStats(updatePercentiles);
            }
            if (SufferCounters &&
                volume.VolumeBase->PerfCalc.IsSuffering())
            {
                SufferCounters->OnDiskSuffer(
                    volume.VolumeBase->Volume.GetStorageMediaKind());
            };
        }

        if (SufferCounters) {
            SufferCounters->PublishCounters();
        }
    }

    TVolumePerfStatuses GatherVolumePerfStatuses() override
    {
        TReadGuard guard(Lock);
        TVolumePerfStatuses ans(Reserve(Volumes.size()));

        for (const auto& [key, info]: Volumes) {
            ans.emplace_back(
                info.VolumeBase->Volume.GetDiskId(),
                info.VolumeBase->PerfCalc.IsSuffering());
        }
        return ans;
    }

    IUserMetricsSupplierPtr GetUserCounters() const override
    {
        return UserCounters;
    }

private:
    const TString& SelectInstanceId(
        const TString& clientId,
        const TString& instanceId)
    {
        // in case of multi mount for empty instance, centers override itself
        // to avoid it use client ID for subgroup
        return instanceId.empty()
            ? clientId
            : instanceId;
    }

    TVolumeInfoHolder RegisterVolume(
        NProto::TVolume volume)
    {
        if (!Counters) {
            InitCounters();
        }

        auto volumeGroup = Counters->GetSubgroup("volume", volume.GetDiskId());

        auto volumeBase = std::make_shared<TVolumeInfoBase>(
            std::move(volume),
            DiagnosticsConfig);
        volumeBase->BusyIdleCalc.Register(*volumeGroup);
        volumeBase->PerfCalc.Register(*volumeGroup, volumeBase->Volume);

        return TVolumeInfoHolder{
            .VolumeBase = std::move(volumeBase),
            .VolumeInfos = {}};
    }

    TVolumeInfoPtr RegisterClient(
        TVolumeBasePtr volumeBase,
        const TString& clientId,
        const TString& instanceId)
    {
        const TString& realInstanceId = SelectInstanceId(
            clientId,
            instanceId);

        auto info = std::make_shared<TVolumeInfo>(
            volumeBase,
            Timer,
            clientId,
            instanceId);

        if (!Counters) {
            InitCounters();
        }

        auto volumeGroup = Counters->GetSubgroup(
            "volume",
            volumeBase->Volume.GetDiskId());
        auto countersGroup = volumeGroup->GetSubgroup(
            "instance",
            realInstanceId);
        info->RequestCounters.Register(*countersGroup);

        NUserCounter::RegisterServerVolumeInstance(
            *UserCounters,
            volumeBase->Volume.GetCloudId(),
            volumeBase->Volume.GetFolderId(),
            volumeBase->Volume.GetDiskId(),
            realInstanceId,
            countersGroup);

        return info;
    }

    void UnregisterClient(
        TVolumeBasePtr volumeBase,
        const TString& clientId,
        const TString& instanceId)
    {
        if (!Counters) {
            InitCounters();
        }

        const TString& realInstanceId = SelectInstanceId(
            clientId,
            instanceId);

        Counters->GetSubgroup("volume", volumeBase->Volume.GetDiskId())->
            RemoveSubgroup("instance", realInstanceId);

        NUserCounter::UnregisterServerVolumeInstance(
            *UserCounters,
            volumeBase->Volume.GetCloudId(),
            volumeBase->Volume.GetFolderId(),
            volumeBase->Volume.GetDiskId(),
            realInstanceId);
    }

    void UnregisterVolume(TVolumeBasePtr volumeBase)
    {
        if (!Counters) {
            InitCounters();
        }

        Counters->RemoveSubgroup("volume", volumeBase->Volume.GetDiskId());
    }

    void InitCounters()
    {
        Counters = Monitoring->GetCounters()->GetSubgroup("counters", "blockstore");
        switch (Type) {
            case EVolumeStatsType::EServerStats: {
                SufferCounters = std::make_unique<TSufferCounters>(
                    Counters->GetSubgroup("component", "server"));

                Counters = Counters->GetSubgroup("component", "server_volume");
                break;
            }
            case EVolumeStatsType::EClientStats: {
                Counters = Counters->GetSubgroup("component", "client_volume");
                break;
            }
        }

        Counters = Counters->GetSubgroup("host", "cluster");
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TVolumeStatsStub final
    : public IVolumeStats
{
    bool MountVolume(
        const NProto::TVolume& volume,
        const TString& clientId,
        const TString& instanceId) override
    {
        Y_UNUSED(volume);
        Y_UNUSED(clientId);
        Y_UNUSED(instanceId);

        return true;
    }

    void UnmountVolume(
        const TString& diskId,
        const TString& clientId) override
    {
        Y_UNUSED(clientId);
        Y_UNUSED(diskId);
    }

    IVolumeInfoPtr GetVolumeInfo(
        const TString& diskId,
        const TString& clientId) const override
    {
        Y_UNUSED(diskId);
        Y_UNUSED(clientId);

        return nullptr;
    }

    ui32 GetBlockSize(const TString& diskId) const override
    {
        Y_UNUSED(diskId);

        return DefaultBlockSize;
    }

    void TrimVolumes() override
    {
    }

    void UpdateStats(bool updatePercentiles) override
    {
        Y_UNUSED(updatePercentiles);
    }

    TVolumePerfStatuses GatherVolumePerfStatuses() override
    {
        return {};
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IVolumeStatsPtr CreateVolumeStats(
    IMonitoringServicePtr monitoring,
    TDiagnosticsConfigPtr diagnosticsConfig,
    TDuration inactiveClientsTimeout,
    EVolumeStatsType type,
    ITimerPtr timer)
{
    Y_VERIFY_DEBUG(diagnosticsConfig);
    return std::make_shared<TVolumeStats>(
        std::move(monitoring),
        inactiveClientsTimeout,
        std::move(diagnosticsConfig),
        type,
        std::move(timer));
}

IVolumeStatsPtr CreateVolumeStats(
    IMonitoringServicePtr monitoring,
    TDuration inactiveClientsTimeout,
    EVolumeStatsType type,
    ITimerPtr timer)
{
    NProto::TDiagnosticsConfig diagnosticsConfig;
    return std::make_shared<TVolumeStats>(
        std::move(monitoring),
        inactiveClientsTimeout,
        std::make_shared<TDiagnosticsConfig>(diagnosticsConfig),
        type,
        std::move(timer));
}


IVolumeStatsPtr CreateVolumeStatsStub()
{
    return std::make_shared<TVolumeStatsStub>();
}

}   // namespace NCloud::NBlockStore
