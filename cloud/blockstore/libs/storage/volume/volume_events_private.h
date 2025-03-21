#pragma once

#include "public.h"
#include "volume_state.h"

#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>

#include <cloud/blockstore/libs/storage/core/request_info.h>
#include <cloud/blockstore/libs/storage/protos/volume.pb.h>

#include <library/cpp/actors/core/scheduler_cookie.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_VOLUME_REQUESTS_PRIVATE(xxx, ...)                           \
    xxx(ResetMountSeqNumber,                __VA_ARGS__)                       \
    xxx(ReadHistory,                        __VA_ARGS__)                       \
    xxx(UpdateDevices,                      __VA_ARGS__)                       \
    xxx(MarkCheckpointRequestCompleted,     __VA_ARGS__)                       \
// BLOCKSTORE_VOLUME_REQUESTS_PRIVATE

////////////////////////////////////////////////////////////////////////////////

struct TEvVolumePrivate
{
    //
    // ProcessUpdateVolumeConfig
    //

    struct TProcessUpdateVolumeConfig
    {
    };

    //
    // AllocateDiskIfNeeded
    //

    struct TAllocateDiskIfNeeded
    {
    };

    //
    // RetryStartPartition
    //

    struct TRetryStartPartition
    {
        const ui64 TabletId;
        NActors::TSchedulerCookieHolder Cookie;

        TRetryStartPartition(ui64 tabletId, NActors::ISchedulerCookie* cookie)
            : TabletId(tabletId)
            , Cookie(cookie)
        {}
    };

    //
    // LogUsageStats
    //

    struct TLogUsageStats
    {
    };

    //
    // ResetMountSeqNumber
    //

    struct TResetMountSeqNumberRequest
    {
        TString ClientId;

        TResetMountSeqNumberRequest(TString clientId)
            : ClientId(std::move(clientId))
        {}
    };

    struct TResetMountSeqNumberResponse
    {
    };

    //
    // AcquireDisk
    //

    struct TAcquireDiskIfNeeded
    {
    };

    //
    // ReadHistory
    //

    struct TReadHistoryRequest
    {
        TInstant Timestamp;
        size_t RecordCount;

        TReadHistoryRequest(
                TInstant timestamp,
                size_t recordCount)
            : Timestamp(timestamp)
            , RecordCount(recordCount)
        {}
    };

    struct TReadHistoryResponse
    {
        TVector<THistoryLogItem> History;
    };

    //
    // UpdateDevices
    //

    struct TUpdateDevicesRequest
    {
        TDevices Devices;
        TMigrations Migrations;
        TVector<TDevices> Replicas;
        TVector<TString> FreshDeviceIds;
        NProto::EVolumeIOMode IOMode;
        TInstant IOModeTs;
        bool MuteIOErrors;

        TUpdateDevicesRequest(
                TDevices devices,
                TMigrations migrations,
                TVector<TDevices> replicas,
                TVector<TString> freshDeviceIds,
                NProto::EVolumeIOMode ioMode,
                TInstant ioModeTs,
                bool muteIOErrors)
            : Devices(std::move(devices))
            , Migrations(std::move(migrations))
            , Replicas(std::move(replicas))
            , FreshDeviceIds(std::move(freshDeviceIds))
            , IOMode(ioMode)
            , IOModeTs(ioModeTs)
            , MuteIOErrors(muteIOErrors)
        {}
    };

    struct TUpdateDevicesResponse
    {};

    //
    // PartStatsSaved
    //

    struct TPartStatsSaved
    {};

    //
    // MarkCheckpointRequestCompleted
    //

    struct TMarkCheckpointRequestCompletedRequest
    {
        TRequestInfoPtr RequestInfo;
        ui64 RequestId;

        TMarkCheckpointRequestCompletedRequest(
                TRequestInfoPtr requestInfo,
                ui64 requestId)
            : RequestInfo(std::move(requestInfo))
            , RequestId(requestId)
        {
        }
    };

    struct TMarkCheckpointRequestCompletedResponse
    {
    };

    //
    // MultipartitionWriteOrZeroCompleted
    //

    struct TMultipartitionWriteOrZeroCompleted
    {
        const ui64 VolumeRequestId;

        TMultipartitionWriteOrZeroCompleted(ui64 volumeRequestId)
            : VolumeRequestId(volumeRequestId)
        {}
    };

    //
    // WriteOrZeroCompleted
    //

    struct TWriteOrZeroCompleted
    {
        const ui64 VolumeRequestId;

        TWriteOrZeroCompleted(ui64 volumeRequestId)
            : VolumeRequestId(volumeRequestId)
        {}
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStorePrivateEvents::VOLUME_START,

        BLOCKSTORE_VOLUME_REQUESTS_PRIVATE(BLOCKSTORE_DECLARE_EVENT_IDS)

        EvUpdateCounters,
        EvUpdateThrottlerState,
        EvProcessUpdateVolumeConfig,
        EvAllocateDiskIfNeeded,
        EvRetryStartPartition,
        EvLogUsageStats,
        EvAcquireDiskIfNeeded,
        EvPartStatsSaved,
        EvMultipartitionWriteOrZeroCompleted,
        EvWriteOrZeroCompleted,

        EvEnd
    };

    static_assert(EvEnd < (int)TBlockStorePrivateEvents::VOLUME_END,
        "EvEnd expected to be < TBlockStorePrivateEvents::VOLUME_END");

    BLOCKSTORE_VOLUME_REQUESTS_PRIVATE(BLOCKSTORE_DECLARE_EVENTS)

    using TEvUpdateCounters = TRequestEvent<TEmpty, EvUpdateCounters>;

    using TEvUpdateThrottlerState = TRequestEvent<TEmpty, EvUpdateThrottlerState>;

    using TEvProcessUpdateVolumeConfig = TRequestEvent<
        TProcessUpdateVolumeConfig,
        EvProcessUpdateVolumeConfig
    >;

    using TEvAllocateDiskIfNeeded = TRequestEvent<
        TAllocateDiskIfNeeded,
        EvAllocateDiskIfNeeded
    >;

    using TEvRetryStartPartition = TRequestEvent<
        TRetryStartPartition,
        EvRetryStartPartition
    >;

    using TEvLogUsageStats = TRequestEvent<
        TLogUsageStats,
        EvLogUsageStats
    >;

    using TEvAcquireDiskIfNeeded = TRequestEvent<
        TAcquireDiskIfNeeded,
        EvAcquireDiskIfNeeded
    >;

    using TEvPartStatsSaved = TRequestEvent<
        TPartStatsSaved,
        EvPartStatsSaved
    >;

    using TEvMultipartitionWriteOrZeroCompleted = TRequestEvent<
        TMultipartitionWriteOrZeroCompleted,
        EvMultipartitionWriteOrZeroCompleted
    >;

    using TEvWriteOrZeroCompleted = TRequestEvent<
        TWriteOrZeroCompleted,
        EvWriteOrZeroCompleted
    >;
};

}   // namespace NCloud::NBlockStore::NStorage
