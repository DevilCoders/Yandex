#pragma once

#include "public.h"

#include "disk_registry_counters.h"
#include "disk_registry_database.h"
#include "disk_registry_private.h"
#include "disk_registry_state.h"
#include "disk_registry_tx.h"

#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/logbroker/public.h>
#include <cloud/blockstore/libs/notify/public.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/pending_request.h>
#include <cloud/blockstore/libs/storage/core/tablet.h>

#include <ydb/core/base/tablet_pipe.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/actors/core/mon.h>

#include <util/generic/deque.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TDiskRegistryActor final
    : public NActors::TActor<TDiskRegistryActor>
    , public TTabletBase<TDiskRegistryActor>
{
    using TLogbrokerServicePtr = NLogbroker::IServicePtr;

    enum EState
    {
        STATE_BOOT,
        STATE_INIT,
        STATE_WORK,
        STATE_RESTORE,
        STATE_ZOMBIE,
        STATE_MAX,
    };

    struct TStateInfo
    {
        TString Name;
        NActors::IActor::TReceiveFunc Func;
    };

private:
    const TStorageConfigPtr Config;
    const TDiagnosticsConfigPtr DiagnosticsConfig;

    static const TStateInfo States[];
    EState CurrentState = STATE_BOOT;

    std::unique_ptr<TDiskRegistryState> State;
    NKikimr::TTabletCountersWithTxTypes* Counters = nullptr;
    NMonitoring::TDynamicCountersPtr ComponentGroup;

    // Pending requests
    TDeque<TPendingRequest> PendingRequests;

    bool BrokenDisksDestructionInProgress = false;
    bool DisksNotificationInProgress = false;
    bool UsersNotificationInProgress = false;
    bool DiskStatesPublicationInProgress = false;
    bool SecureEraseInProgress = false;
    bool StartMigrationInProgress = false;

    TVector<TString> DisksBeingDestroyed;
    TVector<TDiskNotification> DisksBeingNotified;
    TVector<TString> ErrorNotifications;

    TInstant BrokenDisksDestructionStartTs;
    TInstant DisksNotificationStartTs;
    TInstant UsersNotificationStartTs;
    TInstant DiskStatesPublicationStartTs;
    TInstant SecureEraseStartTs;
    TInstant StartMigrationStartTs;

    THashMap<NActors::TActorId, TString> ServerToAgentId;

    struct TAgentRegInfo
    {
        ui64 SeqNo = 0;
        bool Connected = false;
    };

    THashMap<TString, TAgentRegInfo> AgentRegInfo;

    // Requests in-progress
    THashSet<NActors::TActorId> Actors;

    TLogbrokerServicePtr LogbrokerService;
    NNotify::IServicePtr NotifyService;

public:
    TDiskRegistryActor(
        const NActors::TActorId& owner,
        NKikimr::TTabletStorageInfoPtr storage,
        TStorageConfigPtr config,
        TDiagnosticsConfigPtr diagnosticsConfig,
        TLogbrokerServicePtr logbrokerService,
        NNotify::IServicePtr notifyService);

    ~TDiskRegistryActor();

    static constexpr ui32 LogComponent = TBlockStoreComponents::DISK_REGISTRY;

    static TString GetStateName(ui32 state);

private:
    void BecomeAux(const NActors::TActorContext& ctx, EState state);
    void ReportTabletState(const NActors::TActorContext& ctx);

    void OnActivateExecutor(const NActors::TActorContext& ctx) override;

    bool OnRenderAppHtmlPage(
        NKikimr::NMon::TEvRemoteHttpInfo::TPtr ev,
        const NActors::TActorContext& ctx) override;

    void OnDetach(const NActors::TActorContext& ctx) override;

    void OnTabletDead(
        NKikimr::TEvTablet::TEvTabletDead::TPtr& ev,
        const NActors::TActorContext& ctx) override;

    void KillActors(const NActors::TActorContext& ctx);
    void UnregisterCounters(const NActors::TActorContext& ctx);

    void BeforeDie(const NActors::TActorContext& ctx);

    void RegisterCounters(const NActors::TActorContext& ctx);
    void ScheduleCountersUpdate(const NActors::TActorContext& ctx);
    void UpdateCounters(const NActors::TActorContext& ctx);

    void UpdateActorStats(const NActors::TActorContext& ctx);
    void UpdateActorStatsSampled(const NActors::TActorContext& ctx)
    {
        static constexpr int SampleRate = 128;
        if (Y_UNLIKELY(GetHandledEvents() % SampleRate == 0)) {
            UpdateActorStats(ctx);
        }
    }

    void ScheduleCleanup(const NActors::TActorContext& ctx);
    void SecureErase(const NActors::TActorContext& ctx);

    void DestroyBrokenDisks(const NActors::TActorContext& ctx);

    void NotifyDisks(const NActors::TActorContext& ctx);
    void NotifyUsers(const NActors::TActorContext& ctx);

    void UpdateVolumeConfigs(
        const NActors::TActorContext& ctx,
        const TVector<TString>& diskIds);

    void UpdateVolumeConfigs(const NActors::TActorContext& ctx)
    {
        UpdateVolumeConfigs(ctx, State->GetOutdatedVolumeConfigs());
    }

    void FinishVolumeConfigUpdate(
        const NActors::TActorContext& ctx,
        const TString& diskId);

    void UpdateAgentState(
        const NActors::TActorContext& ctx,
        TString agentId,
        NProto::EAgentState state,
        TInstant timestamp,
        bool force);

    void PublishDiskStates(const NActors::TActorContext& ctx);

    void ScheduleRejectAgent(
        const NActors::TActorContext& ctx,
        TString agentId,
        ui64 seqNo);

    void StartMigration(const NActors::TActorContext& ctx);

    bool LoadState(
        TDiskRegistryDatabase& db,
        TTxDiskRegistry::TLoadDBState& args);

    void RejectHttpRequest(const NActors::TActorId& sender, const NActors::TActorContext& ctx);

    void RenderHtmlInfo(IOutputStream& out) const;
    void RenderState(IOutputStream& out) const;
    void RenderDiskList(IOutputStream& out) const;
    void RenderMigrationList(IOutputStream& out) const;
    void RenderBrokenDiskList(IOutputStream& out) const;
    void RenderDisksToNotify(IOutputStream& out) const;
    void RenderErrorNotifications(IOutputStream& out) const;
    void RenderPlacementGroupList(IOutputStream& out) const;
    void RenderRacks(IOutputStream& out) const;
    void RenderAgentList(IOutputStream& out) const;
    void RenderConfig(IOutputStream& out) const;
    void RenderDirtyDeviceList(IOutputStream& out) const;
    void RenderSuspendedDeviceList(IOutputStream& out) const;
    void RenderDeviceHtmlInfo(IOutputStream& out, const TString& id) const;
    void RenderAgentHtmlInfo(IOutputStream& out, const TString& id) const;
    void RenderDiskHtmlInfo(IOutputStream& out, const TString& id) const;

private:
    STFUNC(StateBoot);
    STFUNC(StateInit);
    STFUNC(StateWork);
    STFUNC(StateRestore);
    STFUNC(StateZombie);

    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWakeup(
        const NActors::TEvents::TEvWakeup::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo_VolumeRealloc(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const TString& diskId,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo_ReplaceDevice(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const TString& diskId,
        const TString& deviceId,
        const NActors::TActorContext& ctx);

    void HandleCleanupDisksResponse(
        const TEvDiskRegistryPrivate::TEvCleanupDisksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleDestroyBrokenDisksResponse(
        const TEvDiskRegistryPrivate::TEvDestroyBrokenDisksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleNotifyDisksResponse(
        const TEvDiskRegistryPrivate::TEvNotifyDisksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleNotifyUsersResponse(
        const TEvDiskRegistryPrivate::TEvNotifyUsersResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePublishDiskStatesResponse(
        const TEvDiskRegistryPrivate::TEvPublishDiskStatesResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleSecureEraseResponse(
        const TEvDiskRegistryPrivate::TEvSecureEraseResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleStartMigrationResponse(
        const TEvDiskRegistryPrivate::TEvStartMigrationResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleServerConnected(
        const NKikimr::TEvTabletPipe::TEvServerConnected::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleServerDisconnected(
        const NKikimr::TEvTabletPipe::TEvServerDisconnected::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleAgentConnectionLost(
        const TEvDiskRegistryPrivate::TEvAgentConnectionLost::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleOperationCompleted(
        const TEvDiskRegistryPrivate::TEvOperationCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUpdateVolumeConfigResponse(
        const TEvDiskRegistryPrivate::TEvUpdateVolumeConfigResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    bool HandleRequests(STFUNC_SIG);
    bool RejectRequests(STFUNC_SIG);

    BLOCKSTORE_DISK_REGISTRY_REQUESTS(BLOCKSTORE_IMPLEMENT_REQUEST, TEvDiskRegistry)
    BLOCKSTORE_DISK_REGISTRY_REQUESTS_FWD_SERVICE(BLOCKSTORE_IMPLEMENT_REQUEST, TEvService)
    BLOCKSTORE_DISK_REGISTRY_REQUESTS_PRIVATE(BLOCKSTORE_IMPLEMENT_REQUEST, TEvDiskRegistryPrivate)

    using TCounters = TDiskRegistryCounters;
    BLOCKSTORE_DISK_REGISTRY_TRANSACTIONS(BLOCKSTORE_IMPLEMENT_TRANSACTION, TTxDiskRegistry)
};

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_DISK_REGISTRY_COUNTER(name)                                 \
    if (Counters) {                                                            \
        auto& counter = Counters->Cumulative()                                 \
            [TDiskRegistryCounters::CUMULATIVE_COUNTER_Request_##name];        \
        counter.Increment(1);                                                  \
    }                                                                          \
// BLOCKSTORE_DISK_REGISTRY_COUNTER

bool ToLogicalBlocks(NProto::TDeviceConfig& device, ui32 logicalBlockSize);

}   // namespace NCloud::NBlockStore::NStorage
