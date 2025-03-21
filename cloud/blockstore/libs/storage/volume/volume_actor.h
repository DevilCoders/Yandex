#pragma once

#include "public.h"

#include "tracing.h"
#include "volume.h"
#include "volume_counters.h"
#include "volume_events_private.h"
#include "volume_state.h"
#include "volume_tx.h"

#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/rdma/public.h>
#include <cloud/blockstore/libs/storage/api/bootstrapper.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/partition.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/stats_service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/metrics.h>
#include <cloud/blockstore/libs/storage/core/pending_request.h>
#include <cloud/blockstore/libs/storage/core/tablet.h>
#include <cloud/blockstore/libs/storage/volume/model/requests_inflight.h>
#include <cloud/blockstore/libs/storage/volume/model/volume_throttling_policy.h>

#include <cloud/storage/core/libs/api/hive_proxy.h>
#include <cloud/storage/core/protos/trace.pb.h>

#include <ydb/core/blockstore/core/blockstore.h>
#include <ydb/core/base/tablet_pipe.h>
#include <ydb/core/mind/local.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/actors/core/mon.h>

#include <util/generic/deque.h>
#include <util/generic/list.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TVolumeActor final
    : public NActors::TActor<TVolumeActor>
    , public TTabletBase<TVolumeActor>
{
    enum EState
    {
        STATE_BOOT,
        STATE_INIT,
        STATE_WORK,
        STATE_ZOMBIE,
        STATE_MAX,
    };

    struct TStateInfo
    {
        TString Name;
        NActors::IActor::TReceiveFunc Func;
    };

    enum EStatus
    {
        STATUS_OFFLINE,
        STATUS_INACTIVE,
        STATUS_ONLINE,
        STATUS_MOUNTED
    };

    using TEvUpdateVolumeConfigPtr =
        std::unique_ptr<NKikimr::TEvBlockStore::TEvUpdateVolumeConfig::THandle>;

    struct TClientRequest
    {
        const TRequestInfoPtr RequestInfo;
        const TString DiskId;
        const NActors::TActorId PipeServerActorId;

        const NProto::TVolumeClientInfo AddedClientInfo;
        const TString RemovedClientId;
        const bool IsMonRequest = false;

        TClientRequest(
                TRequestInfoPtr requestInfo,
                TString diskId,
                const NActors::TActorId& pipeServerActorId,
                NProto::TVolumeClientInfo addedClientInfo)
            : RequestInfo(std::move(requestInfo))
            , DiskId(std::move(diskId))
            , PipeServerActorId(pipeServerActorId)
            , AddedClientInfo(std::move(addedClientInfo))
        {}

        TClientRequest(
                TRequestInfoPtr requestInfo,
                TString diskId,
                const NActors::TActorId& pipeServerActorId,
                TString removedClientId,
                bool isMonRequest)
            : RequestInfo(std::move(requestInfo))
            , DiskId(std::move(diskId))
            , PipeServerActorId(pipeServerActorId)
            , RemovedClientId(std::move(removedClientId))
            , IsMonRequest(isMonRequest)
        {}

        TString GetClientId() const
        {
            return RemovedClientId
                ? RemovedClientId
                : AddedClientInfo.GetClientId();
        }
    };

    using TClientRequestPtr = std::shared_ptr<TClientRequest>;

    struct TVolumeRequest
    {
        using TCancelRoutine = std::function<void(
            const NActors::TActorContext& ctx,
            NActors::IEventHandle& request,
            TCallContext& callContext,
            NProto::TError error)>;

        NActors::IEventHandlePtr Request;
        TCallContextPtr CallContext;
        TCallContextPtr ForkedContext;
        ui64 ReceiveTime;
        TCancelRoutine CancelRoutine;

        TVolumeRequest(
                NActors::IEventHandlePtr request,
                TCallContextPtr callContext,
                TCallContextPtr forkedContext,
                ui64 receiveTime,
                TCancelRoutine cancelRoutine)
            : Request(std::move(request))
            , CallContext(std::move(callContext))
            , ForkedContext(std::move(forkedContext))
            , ReceiveTime(receiveTime)
            , CancelRoutine(cancelRoutine)
        {}

        void CancelRequest(
            const NActors::TActorContext& ctx,
            NProto::TError error)
        {
            CancelRoutine(
                ctx,
                *Request,
                *CallContext,
                std::move(error));
        }
    };

    using TVolumeRequestMap = THashMap<ui64, TVolumeRequest>;

private:
    const TStorageConfigPtr Config;
    const TDiagnosticsConfigPtr DiagnosticsConfig;
    const IProfileLogPtr ProfileLog;
    const IBlockDigestGeneratorPtr BlockDigestGenerator;
    const ITraceSerializerPtr TraceSerializer;
    const NRdma::IClientPtr RdmaClient;
    const EVolumeStartMode StartMode;

    std::unique_ptr<TVolumeState> State;
    bool StateLoadFinished = false;

    static const TStateInfo States[];
    EState CurrentState = STATE_BOOT;

    NKikimr::TTabletCountersWithTxTypes* Counters = nullptr;

    bool UpdateCountersScheduled = false;
    bool UpdateLeakyBucketCountersScheduled = false;
    TInstant LastThrottlerStateWrite;

    // Next version that is being applied
    ui32 NextVolumeConfigVersion = 0;

    // Pending UpdateVolumeConfig operations
    TDeque<TEvUpdateVolumeConfigPtr> PendingVolumeConfigUpdates;
    bool ProcessUpdateVolumeConfigScheduled = false;
    bool UpdateVolumeConfigInProgress = false;

    struct TUnfinishedUpdateVolumeConfig
    {
        NKikimrBlockStore::TUpdateVolumeConfig Record;
        ui32 ConfigVersion = 0;
        TRequestInfoPtr RequestInfo;
        TDevices Devices;
        TMigrations Migrations;
        TVector<TDevices> Replicas;
        TVector<TString> FreshDeviceIds;

        void Clear()
        {
            Record.Clear();
            ConfigVersion = 0;
            RequestInfo = nullptr;
        }
    } UnfinishedUpdateVolumeConfig;

    // Pending requests while partitions are not ready
    TDeque<TPendingRequest> PendingRequests;

    TInstant UsageIntervalBegin;

    TDeque<TClientRequestPtr> PendingClientRequests;

    struct TPostponedRequest
    {
        TVolumeThrottlingPolicy::TRequestInfo Info;
        TCallContextPtr CallContext;
        NActors::IEventHandlePtr Event;
    };
    // TODO: replace with a ring buffer
    TList<TPostponedRequest> PostponedRequests;
    bool PostponedQueueFlushScheduled = false;
    bool PostponedQueueFlushInProgress = false;
    bool ShuttingDown = false;

    EPublishingPolicy CountersPolicy = EPublishingPolicy::All;
    TVolumeSelfCountersPtr VolumeSelfCounters;

    struct TAcquireReleaseDiskRequest
    {
        bool IsAcquire;
        TString ClientId;
        NProto::EVolumeAccessMode AccessMode;
        ui64 MountSeqNumber;
        TClientRequestPtr ClientRequest;

        TAcquireReleaseDiskRequest(
                TString clientId,
                NProto::EVolumeAccessMode accessMode,
                ui64 mountSeqNumber,
                TClientRequestPtr clientRequest)
            : IsAcquire(true)
            , ClientId(std::move(clientId))
            , AccessMode(accessMode)
            , MountSeqNumber(mountSeqNumber)
            , ClientRequest(std::move(clientRequest))
        {
        }

        TAcquireReleaseDiskRequest(
                TString clientId,
                TClientRequestPtr clientRequest)
            : IsAcquire(false)
            , ClientId(std::move(clientId))
            , AccessMode(NProto::EVolumeAccessMode::VOLUME_ACCESS_READ_WRITE) // doesn't matter
            , MountSeqNumber(0) // doesn't matter
            , ClientRequest(std::move(clientRequest))
        {
        }
    };
    TList<TAcquireReleaseDiskRequest> AcquireReleaseDiskRequests;
    bool AcquireDiskScheduled = false;

    NProto::TError StorageAllocationResult;
    bool DiskAllocationScheduled = false;

    TVolumeRequestMap VolumeRequests;
    TRequestsInFlight WriteAndZeroRequestsInFlight;

    ui64 VolumeRequestId = 0;

    TIntrusiveList<TRequestInfo> ActiveReadHistoryRequests;
    TInstant LastHistoryCleanup;

    bool PartitionsStartRequested = false;

    struct TCheckpointRequestInfo
    {
        TRequestInfoPtr RequestInfo;
        ui64 RequestId = 0;
        TString CheckpointId;
        ECheckpointRequestType ReqType;
        bool IsTraced;
        ui64 TraceTs;

        TCheckpointRequestInfo(
                TRequestInfoPtr requestInfo,
                ui64 requestId,
                TString checkpointId,
                ECheckpointRequestType reqType,
                bool isTraced,
                ui64 traceTs)
            : RequestInfo(std::move(requestInfo))
            , RequestId(requestId)
            , CheckpointId(std::move(checkpointId))
            , ReqType(reqType)
            , IsTraced(isTraced)
            , TraceTs(traceTs)
        {}
    };
    TList<TCheckpointRequestInfo> CheckpointRequestQueue;
    bool CheckpointRequestInProgress = false;

    ui32 MultipartitionWriteAndZeroRequestsInProgress = 0;

    // requests in progress
    THashSet<NActors::TActorId> Actors;
    TIntrusiveList<TRequestInfo> ActiveTransactions;

    NBlobMetrics::TBlobLoadMetrics PrevMetrics;

    THashSet<NActors::TActorId> StoppedPartitions;

    using TPoisonCallback = std::function<
        void (const NActors::TActorContext&, NProto::TError)
    >;

    TDeque<std::pair<NActors::TActorId, TPoisonCallback>> WaitForPartitions;

public:
    TVolumeActor(
        const NActors::TActorId& owner,
        NKikimr::TTabletStorageInfoPtr storage,
        TStorageConfigPtr config,
        TDiagnosticsConfigPtr diagnosticsConfig,
        IProfileLogPtr profileLog,
        IBlockDigestGeneratorPtr blockDigestGenerator,
        ITraceSerializerPtr traceSerializer,
        NRdma::IClientPtr rdmaClient,
        EVolumeStartMode startMode);
    ~TVolumeActor();

    static constexpr ui32 LogComponent = TBlockStoreComponents::VOLUME;
    using TCounters = TVolumeCounters;

    static TString GetStateName(ui32 state);

private:
    void Enqueue(STFUNC_SIG) override;
    void DefaultSignalTabletActive(const NActors::TActorContext& ctx) override;

    void BecomeAux(const NActors::TActorContext& ctx, EState state);
    void ReportTabletState(const NActors::TActorContext& ctx);

    void RegisterCounters(const NActors::TActorContext& ctx);
    void ScheduleCountersUpdate(const NActors::TActorContext& ctx);
    void UpdateCounters(const NActors::TActorContext& ctx);
    void UpdateLeakyBucketCounters(const NActors::TActorContext& ctx);

    void UpdateActorStats(const NActors::TActorContext& ctx);

    void UpdateActorStatsSampled(const NActors::TActorContext& ctx)
    {
        static constexpr int SampleRate = 128;
        if (Y_UNLIKELY(GetHandledEvents() % SampleRate == 0)) {
            UpdateActorStats(ctx);
        }
    }

    void SendPartStatsToService(const NActors::TActorContext& ctx);
    void SendSelfStatsToService(const NActors::TActorContext& ctx);

    void OnActivateExecutor(const NActors::TActorContext& ctx) override;

    bool ReassignChannelsEnabled() const override;

    bool OnRenderAppHtmlPage(
        NActors::NMon::TEvRemoteHttpInfo::TPtr ev,
        const NActors::TActorContext& ctx) override;

    void RenderHtmlInfo(IOutputStream& out) const;

    void RenderTabletList(IOutputStream& out) const;

    void RenderClientList(IOutputStream& out) const;

    void RenderConfig(IOutputStream& out) const;
    void RenderStatus(IOutputStream& out) const;
    void RenderMigrationStatus(IOutputStream& out) const;
    void RenderMountSeqNumber(IOutputStream& out) const;
    void RenderHistory(
        const TDeque<THistoryLogItem>& history,
        IOutputStream& out) const;
    void RenderCheckpoints(IOutputStream& out) const;
    void RenderTraces(IOutputStream& out) const;
    void RenderCommonButtons(IOutputStream& out) const;

    void OnDetach(const NActors::TActorContext& ctx) override;

    void OnTabletDead(
        NKikimr::TEvTablet::TEvTabletDead::TPtr& ev,
        const NActors::TActorContext& ctx) override;

    void BeforeDie(const NActors::TActorContext& ctx);

    void KillActors(const NActors::TActorContext& ctx);
    void AddTransaction(TRequestInfo& transaction);
    void RemoveTransaction(TRequestInfo& transaction);
    void TerminateTransactions(const NActors::TActorContext& ctx);
    void ReleaseTransactions();

    ui32 GetCurrentConfigVersion() const;

    bool SendBootExternalRequest(
        const NActors::TActorContext& ctx,
        TPartitionInfo& partition);

    void ScheduleRetryStartPartition(
        const NActors::TActorContext& ctx,
        TPartitionInfo& partition);

    void OnServicePipeDisconnect(
        const NActors::TActorContext& ctx,
        const NActors::TActorId& serverId,
        TInstant timestamp);

    void StartPartitionsIfNeeded(const NActors::TActorContext& ctx);
    void StartPartitions(const NActors::TActorContext& ctx);
    void StopPartitions(const NActors::TActorContext& ctx);

    void SetupNonreplicatedPartitions(const NActors::TActorContext& ctx);

    void DumpUsageStats(
        const NActors::TActorContext& ctx,
        TVolumeActor::EStatus status);

    static TString GetVolumeStatusString(EStatus status);
    EStatus GetVolumeStatus() const;

    ui64 GetBlocksCount() const;

    void ProcessNextPendingClientRequest(const NActors::TActorContext& ctx);
    void ProcessNextAcquireReleaseDiskRequest(const NActors::TActorContext& ctx);
    void OnClientListUpdate(const NActors::TActorContext& ctx);

    void Postpone(
        const NActors::TActorContext& ctx,
        TVolumeThrottlingPolicy::TRequestInfo requestInfo,
        TCallContextPtr callContext,
        NActors::IEventHandlePtr ev);

    void UpdateDelayCounter(
        TVolumeThrottlingPolicy::EOpType opType,
        TDuration time);

    void ResetServicePipes(const NActors::TActorContext& ctx);

    void RegisterVolume(const NActors::TActorContext& ctx);
    void UnregisterVolume(const NActors::TActorContext& ctx);
    void SendVolumeConfigUpdated(const NActors::TActorContext& ctx);
    void SendVolumeSelfCounters(const NActors::TActorContext& ctx);

    void SendPendingRequests(const NActors::TActorContext& ctx);

    void ProcessReadHistory(
        const NActors::TActorContext& ctx,
        TRequestInfoPtr requestInfo,
        TInstant ts,
        size_t recordCount,
        bool monRequest);

    bool CheckAllocationResult(
        const NActors::TActorContext& ctx,
        const TDevices& devices,
        const TVector<TDevices>& replicas);

    void CopyCachedStatsToPartCounters(
        const NProto::TCachedPartStats& src,
        TPartitionDiskCounters& dst);

    void CopyPartCountersToCachedStats(
        const TPartitionDiskCounters& src,
        NProto::TCachedPartStats& dst);

    void UpdateCachedStats(
        const TPartitionDiskCounters& src,
        TPartitionDiskCounters& dst);

    void SetupIOErrorDetails(const NActors::TActorContext& ctx);

    void CancelRequests(const NActors::TActorContext& ctx);

    void RejectHttpRequest(const NActors::TActorId& sender, const NActors::TActorContext& ctx);

private:
    STFUNC(StateBoot);
    STFUNC(StateInit);
    STFUNC(StateWork);
    STFUNC(StateZombie);

    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePoisonTaken(
        const NActors::TEvents::TEvPoisonTaken::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo_RemoveClient(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const TString& clientId,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo_ResetMountSeqNumber(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const TString& clientId,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo_CreateCheckpoint(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const TString& checkpointId,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo_DeleteCheckpoint(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const TString& checkpointId,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo_StartPartitions(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo_Default(
        const NActors::TActorContext& ctx,
        const TDeque<THistoryLogItem>& history,
        const TStringBuf tabName,
        IOutputStream& out);

    void HandleUpdateThrottlerState(
        const TEvVolumePrivate::TEvUpdateThrottlerState::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUpdateCounters(
        const TEvVolumePrivate::TEvUpdateCounters::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleNonreplicatedPartCounters(
        const TEvVolume::TEvNonreplicatedPartitionCounters::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePartCounters(
        const TEvStatsService::TEvVolumePartCounters::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePartStatsSaved(
        const TEvVolumePrivate::TEvPartStatsSaved::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleMultipartitionWriteOrZeroCompleted(
        const TEvVolumePrivate::TEvMultipartitionWriteOrZeroCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWriteOrZeroCompleted(
        const TEvVolumePrivate::TEvWriteOrZeroCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void ProcessNextCheckpointRequest(const NActors::TActorContext& ctx);

    void HandleTabletStatus(
        const TEvBootstrapper::TEvStatus::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUpdateVolumeConfig(
        const NKikimr::TEvBlockStore::TEvUpdateVolumeConfig::TPtr& ev,
        const NActors::TActorContext& ctx);

    bool UpdateVolumeConfig(
        const NKikimr::TEvBlockStore::TEvUpdateVolumeConfig::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleAcquireDiskResponse(
        const TEvDiskRegistry::TEvAcquireDiskResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void AcquireDisk(
        const NActors::TActorContext& ctx,
        TString clientId,
        NProto::EVolumeAccessMode accessMode,
        ui64 mountSeqNumber);

    void AcquireDiskIfNeeded(const NActors::TActorContext& ctx);

    void ScheduleAcquireDiskIfNeeded(const NActors::TActorContext& ctx);

    void HandleAcquireDiskIfNeeded(
        const TEvVolumePrivate::TEvAcquireDiskIfNeeded::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleReacquireDisk(
        const TEvVolume::TEvReacquireDisk::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleRdmaUnavailable(
        const TEvVolume::TEvRdmaUnavailable::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleReleaseDiskResponse(
        const TEvDiskRegistry::TEvReleaseDiskResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void ReleaseDisk(const NActors::TActorContext& ctx, const TString& clientId);

    void HandleAllocateDiskResponse(
        const TEvDiskRegistry::TEvAllocateDiskResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void ScheduleAllocateDiskIfNeeded(const NActors::TActorContext& ctx);

    NProto::TAllocateDiskRequest MakeAllocateDiskRequest() const;

    void AllocateDisk(const NActors::TActorContext& ctx);

    void HandleAllocateDiskIfNeeded(
        const TEvVolumePrivate::TEvAllocateDiskIfNeeded::TPtr& ev,
        const NActors::TActorContext& ctx);

    void ScheduleProcessUpdateVolumeConfig(const NActors::TActorContext& ctx);

    void HandleProcessUpdateVolumeConfig(
        const TEvVolumePrivate::TEvProcessUpdateVolumeConfig::TPtr& ev,
        const NActors::TActorContext& ctx);

    void FinishUpdateVolumeConfig(const NActors::TActorContext& ctx);

    const NKikimrBlockStore::TVolumeConfig& GetNewestConfig() const;

    void HandleRetryStartPartition(
        const TEvVolumePrivate::TEvRetryStartPartition::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleBootExternalResponse(
        const NCloud::NStorage::TEvHiveProxy::TEvBootExternalResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWaitReadyResponse(
        const NPartition::TEvPartition::TEvWaitReadyResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleLogUsageStats(
        const TEvVolumePrivate::TEvLogUsageStats::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleBackpressureReport(
        const NPartition::TEvPartition::TEvBackpressureReport::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWakeup(
        const NActors::TEvents::TEvWakeup::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleServerConnected(
        const NKikimr::TEvTabletPipe::TEvServerConnected::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleServerDisconnected(
        const NKikimr::TEvTabletPipe::TEvServerDisconnected::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleServerDestroyed(
        const NKikimr::TEvTabletPipe::TEvServerDestroyed::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleTabletMetrics(
        NKikimr::TEvLocal::TEvTabletMetrics::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUpdateMigrationState(
        const TEvVolume::TEvUpdateMigrationState::TPtr& ev,
        const NActors::TActorContext& ctx);

    template <typename TMethod>
    void FillResponse(
        typename TMethod::TResponse& response,
        TCallContext& callContext,
        ui64 startTime);

    template <typename TMethod>
    void SendRequestToPartition(
        const NActors::TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev,
        ui32 partitionId,
        ui64 traceTs);

    template <typename TMethod>
    void SendRequestToNonreplicatedPartitions(
        const NActors::TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev,
        const NActors::TActorId& partitions,
        bool isTraced);

    template <typename TMethod>
    bool SendRequestToPartitionWithUsedBlockTracking(
        const NActors::TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev,
        const NActors::TActorId& partitions);

    template <typename TMethod>
    bool HandleRequest(
        const NActors::TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev,
        bool isTraced,
        ui64 traceTs);

    template <typename TMethod>
    void ForwardRequest(
        const NActors::TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev);

    template <typename TMethod>
    NProto::TError Throttle(
        const NActors::TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev,
        bool throttlingDisabled);

    template <typename TMethod>
    void HandleResponse(
        const NActors::TActorContext& ctx,
        const typename TMethod::TResponse::TPtr& ev);

    bool HandleRequests(STFUNC_SIG);
    bool RejectRequests(STFUNC_SIG);

    void HandleDescribeBlocksResponse(
        const TEvVolume::TEvDescribeBlocksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleGetUsedBlocksResponse(
        const TEvVolume::TEvGetUsedBlocksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleGetPartitionInfoResponse(
        const TEvVolume::TEvGetPartitionInfoResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleCompactRangeResponse(
        const TEvVolume::TEvCompactRangeResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleGetCompactionStatusResponse(
        const TEvVolume::TEvGetCompactionStatusResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleRebuildMetadataResponse(
        const TEvVolume::TEvRebuildMetadataResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleGetRebuildMetadataStatusResponse(
        const TEvVolume::TEvGetRebuildMetadataStatusResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleReadBlocksResponse(
        const TEvService::TEvReadBlocksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWriteBlocksResponse(
        const TEvService::TEvWriteBlocksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleZeroBlocksResponse(
        const TEvService::TEvZeroBlocksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleCreateCheckpointResponse(
        const TEvService::TEvCreateCheckpointResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleDeleteCheckpointResponse(
        const TEvService::TEvDeleteCheckpointResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleDeleteCheckpointDataResponse(
        const TEvVolume::TEvDeleteCheckpointDataResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWriteBlocksLocalResponse(
        const TEvService::TEvWriteBlocksLocalResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleGetChangedBlocksResponse(
        const TEvService::TEvGetChangedBlocksResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleReadBlocksLocalResponse(
        const TEvService::TEvReadBlocksLocalResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    BLOCKSTORE_VOLUME_REQUESTS(BLOCKSTORE_IMPLEMENT_REQUEST, TEvVolume)
    BLOCKSTORE_VOLUME_REQUESTS_PRIVATE(BLOCKSTORE_IMPLEMENT_REQUEST, TEvVolumePrivate)
    BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE(BLOCKSTORE_IMPLEMENT_REQUEST, TEvService)

    BLOCKSTORE_VOLUME_TRANSACTIONS(BLOCKSTORE_IMPLEMENT_TRANSACTION, TTxVolume)
};

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_VOLUME_COUNTER(name)                                        \
    if (Counters) {                                                            \
        auto& counter = Counters->Cumulative()                                 \
            [TVolumeCounters::CUMULATIVE_COUNTER_Request_##name];              \
        counter.Increment(1);                                                  \
    }                                                                          \
// BLOCKSTORE_VOLUME_COUNTER

}   // namespace NCloud::NBlockStore::NStorage
