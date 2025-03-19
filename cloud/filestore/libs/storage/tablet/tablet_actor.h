#pragma once

#include "public.h"

#include "tablet_counters.h"
#include "tablet_database.h"
#include "tablet_private.h"
#include "tablet_state.h"
#include "tablet_tx.h"

#include <cloud/filestore/libs/diagnostics/filesystem_counters.h>
#include <cloud/filestore/libs/diagnostics/public.h>
#include <cloud/filestore/libs/storage/api/service.h>
#include <cloud/filestore/libs/storage/api/tablet.h>
#include <cloud/filestore/libs/storage/core/config.h>
#include <cloud/filestore/libs/storage/core/tablet.h>
#include <cloud/filestore/libs/storage/core/utils.h>
#include <cloud/filestore/libs/storage/tablet/model/range.h>

#include <cloud/storage/core/libs/diagnostics/public.h>

#include <ydb/core/base/tablet_pipe.h>
#include <ydb/core/mind/local.h>
#include <ydb/core/filestore/core/filestore.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/actors/core/mon.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TIndexTabletActor final
    : public NActors::TActor<TIndexTabletActor>
    , public TTabletBase<TIndexTabletActor>
    , public TIndexTabletState
{
    static constexpr size_t MaxBlobStorageBlobSize = 40 * 1024 * 1024;

    enum EState
    {
        STATE_BOOT,
        STATE_INIT,
        STATE_WORK,
        STATE_ZOMBIE,
        STATE_BROKEN,
        STATE_MAX,
    };

    struct TStateInfo
    {
        TString Name;
        NActors::IActor::TReceiveFunc Func;
    };

private:
    const TStorageConfigPtr Config;
    const IProfileLogPtr ProfileLog;

    static const TStateInfo States[];
    EState CurrentState = STATE_BOOT;

    IStorageCountersPtr StorageCounters;
    TFileSystemStatCountersPtr StatCounters;

    NKikimr::TTabletCountersWithTxTypes* Counters = nullptr;
    bool UpdateCountersScheduled = false;
    bool CleanupSessionsScheduled = false;

    TDeque<NActors::IEventHandlePtr> WaitReadyRequests;

    TSet<NActors::TActorId> WorkerActors;

    TInstant ReassignRequestSentTs;

public:
    TIndexTabletActor(
        const NActors::TActorId& owner,
        NKikimr::TTabletStorageInfoPtr storage,
        TStorageConfigPtr config,
        IProfileLogPtr profileLog,
        IStorageCountersPtr storageCounters);
    ~TIndexTabletActor();

    static constexpr ui32 LogComponent = TFileStoreComponents::TABLET;
    using TCounters = TIndexTabletCounters;

    static TString GetStateName(ui32 state);

private:
    void Enqueue(STFUNC_SIG) override;
    void DefaultSignalTabletActive(const NActors::TActorContext& ctx) override;
    void OnActivateExecutor(const NActors::TActorContext& ctx) override;
    bool ReassignChannelsEnabled() const override;
    void ReassignDataChannelsIfNeeded(const NActors::TActorContext& ctx);
    bool OnRenderAppHtmlPage(
        NActors::NMon::TEvRemoteHttpInfo::TPtr ev,
        const NActors::TActorContext& ctx) override;
    void OnDetach(const NActors::TActorContext& ctx) override;
    void OnTabletDead(
        NKikimr::TEvTablet::TEvTabletDead::TPtr& ev,
        const NActors::TActorContext& ctx) override;

    void Suicide(const NActors::TActorContext& ctx);
    void BecomeAux(const NActors::TActorContext& ctx, EState state);
    void ReportTabletState(const NActors::TActorContext& ctx);

    void RegisterStatCounters();
    void RegisterCounters(const NActors::TActorContext& ctx);
    void ScheduleUpdateCounters(const NActors::TActorContext& ctx);
    void UpdateCounters();

    void ScheduleCleanupSessions(const NActors::TActorContext& ctx);
    void RestartCheckpointDestruction(const NActors::TActorContext& ctx);

    void EnqueueWriteBatch(
        const NActors::TActorContext& ctx,
        std::unique_ptr<TWriteRequest> request);

    void EnqueueFlushIfNeeded(const NActors::TActorContext& ctx);
    void EnqueueBlobIndexOpIfNeeded(const NActors::TActorContext& ctx);
    void EnqueueCollectGarbageIfNeeded(const NActors::TActorContext& ctx);

    void NotifySessionEvent(
        const NActors::TActorContext& ctx,
        const NProto::TSessionEvent& event);

    NProto::TError ValidateRangeRequest(TByteRange byteRange) const;
    NProto::TError ValidateDataRequest(TByteRange byteRange) const;

    TBackpressureThresholds BuildBackpressureThresholds() const;

    void ExecuteTx_AddBlob_Write(
        const NActors::TActorContext& ctx,
        NKikimr::NTabletFlatExecutor::TTransactionContext& tx,
        TTxIndexTablet::TAddBlob& args);

    void ExecuteTx_AddBlob_Flush(
        const NActors::TActorContext& ctx,
        NKikimr::NTabletFlatExecutor::TTransactionContext& tx,
        TTxIndexTablet::TAddBlob& args);

    void ExecuteTx_AddBlob_FlushBytes(
        const NActors::TActorContext& ctx,
        NKikimr::NTabletFlatExecutor::TTransactionContext& tx,
        TTxIndexTablet::TAddBlob& args);

    void ExecuteTx_AddBlob_Compaction(
        const NActors::TActorContext& ctx,
        NKikimr::NTabletFlatExecutor::TTransactionContext& tx,
        TTxIndexTablet::TAddBlob& args);

private:
    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo(
        const NActors::NMon::TEvRemoteHttpInfo::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleSessionDisconnected(
        const NKikimr::TEvTabletPipe::TEvServerDisconnected::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleTabletMetrics(
        const NKikimr::TEvLocal::TEvTabletMetrics::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUpdateConfig(
        const NKikimr::TEvFileStore::TEvUpdateConfig::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUpdateCounters(
        const TEvIndexTabletPrivate::TEvUpdateCounters::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleCleanupSessionsCompleted(
        const TEvIndexTabletPrivate::TEvCleanupSessionsCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleReadDataCompleted(
        const TEvIndexTabletPrivate::TEvReadDataCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWriteDataCompleted(
        const TEvIndexTabletPrivate::TEvWriteDataCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWriteBatchCompleted(
        const TEvIndexTabletPrivate::TEvWriteBatchCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleReadBlobCompleted(
        const TEvIndexTabletPrivate::TEvReadBlobCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWriteBlobCompleted(
        const TEvIndexTabletPrivate::TEvWriteBlobCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleFlushCompleted(
        const TEvIndexTabletPrivate::TEvFlushCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleFlushBytesCompleted(
        const TEvIndexTabletPrivate::TEvFlushBytesCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleCompactionCompleted(
        const TEvIndexTabletPrivate::TEvCompactionCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleCollectGarbageCompleted(
        const TEvIndexTabletPrivate::TEvCollectGarbageCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleDestroyCheckpointCompleted(
        const TEvIndexTabletPrivate::TEvDestroyCheckpointCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    bool HandleRequests(STFUNC_SIG);
    bool RejectRequests(STFUNC_SIG);
    bool RejectRequestsByBrokenTablet(STFUNC_SIG);

    FILESTORE_SERVICE_REQUESTS_FWD(FILESTORE_IMPLEMENT_REQUEST, TEvService)

    FILESTORE_TABLET_REQUESTS(FILESTORE_IMPLEMENT_REQUEST, TEvIndexTablet)
    FILESTORE_TABLET_REQUESTS_PRIVATE(FILESTORE_IMPLEMENT_REQUEST, TEvIndexTabletPrivate)

    FILESTORE_TABLET_TRANSACTIONS(FILESTORE_IMPLEMENT_TRANSACTION, TTxIndexTablet)

    STFUNC(StateBoot);
    STFUNC(StateInit);
    STFUNC(StateWork);
    STFUNC(StateZombie);
    STFUNC(StateBroken);

    TString LogTag() const;
};

}   // namespace NCloud::NFileStore::NStorage
