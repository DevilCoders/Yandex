#pragma once

#include "public.h"

#include "hive_proxy_events_private.h"

#include <cloud/storage/core/libs/kikimr/helpers.h>
#include <cloud/storage/core/libs/api/hive_proxy.h>

#include <ydb/core/base/hive.h>
#include <ydb/core/mind/local.h>
#include <ydb/core/tablet/tablet_pipe_client_cache.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

namespace NCloud::NStorage {

////////////////////////////////////////////////////////////////////////////////

class THiveProxyActor final
    : public NActors::TActorBootstrapped<THiveProxyActor>
{
public:
    struct TRequestInfo
    {
        NActors::TActorId Sender;
        ui64 Cookie = 0;

        TRequestInfo() = default;
        TRequestInfo(const TRequestInfo&) = default;
        TRequestInfo& operator=(const TRequestInfo&) = default;

        TRequestInfo(NActors::TActorId sender, ui64 cookie)
            : Sender(sender)
            , Cookie(cookie)
        {
        }

        void Drop()
        {
            Sender = {};
            Cookie = 0;
        }

        explicit operator bool() const
        {
            return !!Sender;
        }
    };

private:
    enum ELockPhase
    {
        PHASE_LOCKING,
        PHASE_LOCKED,
        PHASE_RECONNECT,
        PHASE_UNLOCKING,
    };

    struct TLockState
    {
        NActors::TActorId Owner;
        ui64 Cookie = 0;
        ui64 TabletId = 0;
        ELockPhase Phase = PHASE_LOCKING;
        TRequestInfo LockRequest;
        TRequestInfo UnlockRequest;
    };

    class TTabletStats
    {
        enum StatsUpdateState
        {
            STATE_NO_STATS,
            STATE_HAS_STATS,
            STATE_SEND_UPDATE
        };

    private:
        StatsUpdateState State = STATE_NO_STATS;

    public:
        NKikimrTabletBase::TMetrics ResourceValues;
        ui32 SlaveID = 0;

        void OnStatsUpdate()
        {
            switch (State) {
                case STATE_NO_STATS: State = STATE_HAS_STATS; break;
                default: State = STATE_SEND_UPDATE; break;
            }
        }

        void OnHiveAck()
        {
            Y_VERIFY_DEBUG(State != STATE_NO_STATS);

            switch (State) {
                case STATE_HAS_STATS: State = STATE_NO_STATS; break;
                case STATE_SEND_UPDATE: State = STATE_HAS_STATS; break;
                default: break;
            }
        }

        bool IsEmpty() const
        {
            return State == STATE_NO_STATS;
        }
    };

    using TGetInfoRequests = TDeque<TRequestInfo>;

    using TTabletKey = std::pair<ui64, ui64>;

    using TCreateRequest = std::pair<TRequestInfo, bool>;

    struct THiveState
    {
        THashMap<ui64, TLockState> LockStates;
        THashMap<ui64, TGetInfoRequests> GetInfoRequests;
        THashMap<TTabletKey, TVector<TCreateRequest>> CreateRequests;
        THashSet<NActors::TActorId> Actors;
        THashMap<ui64, TTabletStats> UpdatedTabletMetrics;
        bool ScheduledSendTabletMetrics = false;
    };

private:
    std::unique_ptr<NKikimr::NTabletPipe::IClientCache> ClientCache;
    THashMap<ui64, THiveState> HiveStates;
    TDuration LockExpireTimeout;
    int LogComponent = 0;
    IFileIOServicePtr FileIOService;
    TString TabletBootInfoCacheFilePath;
    NActors::TActorId TabletBootInfoCache;

    static constexpr int BatchTimeout = 2000; // ms

public:
    THiveProxyActor(THiveProxyConfig config, IFileIOServicePtr fileIO);

    void Bootstrap(const NActors::TActorContext& ctx);

private:
    STFUNC(StateWork);

    ui64 GetHive(
        const NActors::TActorContext& ctx,
        ui64 tabletId,
        ui32 hiveIdx = Max());

    void SendLockReply(
        const NActors::TActorContext& ctx,
        TLockState* state,
        const NProto::TError& error = {});

    void SendUnlockReply(
        const NActors::TActorContext& ctx,
        TLockState* state,
        const NProto::TError& error = {});

    void SendLockLostNotification(
        const NActors::TActorContext& ctx,
        TLockState* state,
        const NProto::TError& error = {});

    void ScheduleSendTabletMetrics(const NActors::TActorContext& ctx, ui64 hive);

    void AddTabletMetrics(
        ui64 tabletId,
        const TTabletStats& tabletData,
        NKikimrHive::TEvTabletMetrics& record);

    void SendTabletMetrics(
        const NActors::TActorContext& ctx,
        ui64 hiveId);

    void HandleConnect(
        NKikimr::TEvTabletPipe::TEvClientConnected::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleDisconnect(
        NKikimr::TEvTabletPipe::TEvClientDestroyed::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleConnectionError(
        const NActors::TActorContext& ctx,
        const NProto::TError& error,
        ui64 hive,
        bool connectFailed);

    void SendLockRequest(
        const NActors::TActorContext& ctx,
        ui64 hive,
        ui64 tabletId,
        bool reconnect = false);

    void HandleLockTabletExecutionResult(
        const NKikimr::TEvHive::TEvLockTabletExecutionResult::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleLockTabletExecutionLost(
        const NKikimr::TEvHive::TEvLockTabletExecutionLost::TPtr& ev,
        const NActors::TActorContext& ctx);

    void SendUnlockRequest(
        const NActors::TActorContext& ctx,
        ui64 hive,
        ui64 tabletId);

    void HandleUnlockTabletExecutionResult(
        const NKikimr::TEvHive::TEvUnlockTabletExecutionResult::TPtr& ev,
        const NActors::TActorContext& ctx);

    void SendGetTabletStorageInfoRequest(
        const NActors::TActorContext& ctx,
        ui64 hive,
        ui64 tabletId);

    void HandleGetTabletStorageInfoRegistered(
        const NKikimr::TEvHive::TEvGetTabletStorageInfoRegistered::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleGetTabletStorageInfoResult(
        const NKikimr::TEvHive::TEvGetTabletStorageInfoResult::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleRequestFinished(
        const TEvHiveProxyPrivate::TEvRequestFinished::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleTabletMetrics(
        const NKikimr::TEvLocal::TEvTabletMetrics::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleSendTabletMetrics(
        const TEvHiveProxyPrivate::TEvSendTabletMetrics::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleMetricsResponse(
        const NKikimr::TEvLocal::TEvTabletMetricsAck::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleCreateTabletReply(
        const NKikimr::TEvHive::TEvCreateTabletReply::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleTabletCreation(
        const NKikimr::TEvHive::TEvTabletCreationResult::TPtr& ev,
        const NActors::TActorContext& ctx);

    bool HandleRequests(STFUNC_SIG);

    STORAGE_HIVE_PROXY_REQUESTS(STORAGE_IMPLEMENT_REQUEST, TEvHiveProxy)
};

}   // namespace NCloud::NStorage
