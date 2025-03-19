#pragma once

#include "volume_database.h"
#include "volume_state.h"

#include <cloud/blockstore/libs/common/compressed_bitmap.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>
#include <cloud/blockstore/libs/storage/protos/volume.pb.h>

#include <cloud/blockstore/libs/kikimr/trace.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_VOLUME_TRANSACTIONS(xxx, ...)                               \
    xxx(InitSchema,                     __VA_ARGS__)                           \
    xxx(LoadState,                      __VA_ARGS__)                           \
    xxx(UpdateConfig,                   __VA_ARGS__)                           \
    xxx(UpdateDevices,                  __VA_ARGS__)                           \
    xxx(UpdateMigrationState,           __VA_ARGS__)                           \
    xxx(AddClient,                      __VA_ARGS__)                           \
    xxx(RemoveClient,                   __VA_ARGS__)                           \
    xxx(ResetMountSeqNumber,            __VA_ARGS__)                           \
    xxx(ReadHistory,                    __VA_ARGS__)                           \
    xxx(CleanupHistory,                 __VA_ARGS__)                           \
    xxx(SavePartStats,                  __VA_ARGS__)                           \
    xxx(SaveCheckpointRequest,          __VA_ARGS__)                           \
    xxx(MarkCheckpointRequestCompleted, __VA_ARGS__)                           \
    xxx(UpdateUsedBlocks,               __VA_ARGS__)                           \
    xxx(WriteThrottlerState,            __VA_ARGS__)                           \
// BLOCKSTORE_VOLUME_TRANSACTIONS

////////////////////////////////////////////////////////////////////////////////

struct TTxVolume
{
    //
    // InitSchema
    //

    struct TInitSchema
    {
        const TRequestInfoPtr RequestInfo;

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // LoadState
    //

    struct TLoadState
    {
        const TRequestInfoPtr RequestInfo;
        const TInstant OldestLogEntry;

        TMaybe<NProto::TVolumeMeta> Meta;
        THashMap<TString, TVolumeClientState> Clients;
        ui64 MountSeqNumber;
        TVector<THistoryLogKey> OutdatedHistory;
        TDeque<THistoryLogItem> History;
        TVector<TVolumeDatabase::TPartStats> PartStats;
        TVector<TCheckpointRequest> CheckpointRequests;
        THashMap<TString, TInstant> DeletedCheckpoints;
        TVector<ui64> OutdatedCheckpointRequestIds;
        TMaybe<TCompressedBitmap> UsedBlocks;
        TMaybe<TVolumeDatabase::TThrottlerStateInfo> ThrottlerStateInfo;

        TLoadState(TInstant oldestLogEntry)
            : OldestLogEntry(oldestLogEntry)
            , MountSeqNumber(0)
        {}

        void Clear()
        {
            Meta.Clear();
            Clients.clear();
            MountSeqNumber = 0;
            OutdatedHistory.clear();
            History.clear();
            PartStats.clear();
            CheckpointRequests.clear();
            DeletedCheckpoints.clear();
            OutdatedCheckpointRequestIds.clear();
            UsedBlocks.Clear();
            ThrottlerStateInfo.Clear();
        }
    };

    //
    // UpdateConfig
    //

    struct TUpdateConfig
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 TxId;
        const NProto::TVolumeMeta Meta;

        TUpdateConfig(
                TRequestInfoPtr requestInfo,
                ui64 txId,
                NProto::TVolumeMeta meta)
            : RequestInfo(std::move(requestInfo))
            , TxId(txId)
            , Meta(std::move(meta))
        {}

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // UpdateDevices
    //

    struct TUpdateDevices
    {
        const TRequestInfoPtr RequestInfo;
        TDevices Devices;
        TMigrations Migrations;
        TVector<TDevices> Replicas;
        TVector<TString> FreshDeviceIds;
        NProto::EVolumeIOMode IOMode;
        TInstant IOModeTs;
        bool MuteIOErrors;

        TUpdateDevices(
                TDevices devices,
                TMigrations migrations,
                TVector<TDevices> replicas,
                TVector<TString> freshDeviceIds,
                NProto::EVolumeIOMode ioMode,
                TInstant ioModeTs,
                bool muteIOErrors)
            : TUpdateDevices(
                TRequestInfoPtr(),
                std::move(devices),
                std::move(migrations),
                std::move(replicas),
                std::move(freshDeviceIds),
                ioMode,
                ioModeTs,
                muteIOErrors
            )
        {}

        TUpdateDevices(
                TRequestInfoPtr requestInfo,
                TDevices devices,
                TMigrations migrations,
                TVector<TDevices> replicas,
                TVector<TString> freshDeviceIds,
                NProto::EVolumeIOMode ioMode,
                TInstant ioModeTs,
                bool muteIOErrors)
            : RequestInfo(std::move(requestInfo))
            , Devices(std::move(devices))
            , Migrations(std::move(migrations))
            , Replicas(std::move(replicas))
            , FreshDeviceIds(std::move(freshDeviceIds))
            , IOMode(ioMode)
            , IOModeTs(ioModeTs)
            , MuteIOErrors(muteIOErrors)
        {}

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // UpdateMigrationState
    //

    struct TUpdateMigrationState
    {
        const TRequestInfoPtr RequestInfo;
        ui64 MigrationIndex;

        TUpdateMigrationState(TRequestInfoPtr requestInfo, ui64 migrationIndex)
            : RequestInfo(std::move(requestInfo))
            , MigrationIndex(migrationIndex)
        {}

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // AddClient
    //

    struct TAddClient
    {
        const TRequestInfoPtr RequestInfo;
        const TString DiskId;
        const NActors::TActorId PipeServerActorId;
        NProto::TVolumeClientInfo Info;

        NProto::TError Error;

        TAddClient(
                TRequestInfoPtr requestInfo,
                TString diskId,
                const NActors::TActorId& pipeServerActorId,
                NProto::TVolumeClientInfo info)
            : RequestInfo(std::move(requestInfo))
            , DiskId(std::move(diskId))
            , PipeServerActorId(pipeServerActorId)
            , Info(std::move(info))
        {}

        void Clear()
        {
            Error.Clear();
        }
    };

    //
    // RemoveClient
    //

    struct TRemoveClient
    {
        const TRequestInfoPtr RequestInfo;
        const TString DiskId;
        const NActors::TActorId PipeServerActorId;
        const TString ClientId;
        const bool IsMonRequest;

        NProto::TError Error;

        TRemoveClient(
                TRequestInfoPtr requestInfo,
                TString diskId,
                const NActors::TActorId& pipeServerActorId,
                TString clientId,
                bool isMonRequest)
            : RequestInfo(std::move(requestInfo))
            , DiskId(std::move(diskId))
            , PipeServerActorId(pipeServerActorId)
            , ClientId(std::move(clientId))
            , IsMonRequest(isMonRequest)
        {}

        void Clear()
        {
            Error.Clear();
        }
    };

    //
    // Reset MountSeqNumber
    //

    struct TResetMountSeqNumber
    {
        const TRequestInfoPtr RequestInfo;
        const TString ClientId;
        TMaybe<NProto::TVolumeClientInfo> ClientInfo;

        TResetMountSeqNumber(
                TRequestInfoPtr requestInfo,
                TString clientId)
            : RequestInfo(std::move(requestInfo))
            , ClientId(std::move(clientId))
        {}

        void Clear()
        {
            ClientInfo.Clear();
        }
    };

    //
    // Read History
    //

    struct TReadHistory
    {
        const TRequestInfoPtr RequestInfo;
        const TInstant Ts;
        const TInstant OldestTs;
        const size_t RecordCount;
        const bool MonRequest;

        TVector<THistoryLogItem> History;

        TReadHistory(
                TRequestInfoPtr requestInfo,
                TInstant ts,
                TInstant oldestTs,
                size_t recordCount,
                bool monRequest)
            : RequestInfo(std::move(requestInfo))
            , Ts(ts)
            , OldestTs(oldestTs)
            , RecordCount(recordCount)
            , MonRequest(monRequest)
        {}

        void Clear()
        {
            History.clear();
        }
    };

    //
    // Cleanup History
    //

    struct TCleanupHistory
    {
        const TRequestInfoPtr RequestInfo;
        const TInstant Key;

        TVector<THistoryLogKey> OutdatedHistory;

        TCleanupHistory(
                TRequestInfoPtr requestInfo,
                TInstant key)
            : RequestInfo(std::move(requestInfo))
            , Key(key)
        {}

        void Clear()
        {
            OutdatedHistory.clear();
        }
    };

    //
    // SavePartStats
    //

    struct TSavePartStats
    {
        const TRequestInfoPtr RequestInfo;
        const TVolumeDatabase::TPartStats PartStats;
        const bool IsReplicatedVolume;

        TSavePartStats(
                TRequestInfoPtr requestInfo,
                TVolumeDatabase::TPartStats partStats,
                bool isReplicatedVolume)
            : RequestInfo(std::move(requestInfo))
            , PartStats(std::move(partStats))
            , IsReplicatedVolume(isReplicatedVolume)
        {}

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // SaveCheckpointRequest
    //

    struct TSaveCheckpointRequest
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 RequestId;
        const TString CheckpointId;
        const ECheckpointRequestType ReqType;
        const bool IsTraced;
        const ui64 TraceTs;

        TSaveCheckpointRequest(
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

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // MarkCheckpointRequestCompleted
    //

    struct TMarkCheckpointRequestCompleted
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 RequestId;

        TMarkCheckpointRequestCompleted(
                TRequestInfoPtr requestInfo,
                ui64 requestId)
            : RequestInfo(std::move(requestInfo))
            , RequestId(requestId)
        {}

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // UpdateUsedBlocks
    //

    struct TUpdateUsedBlocks
    {
        const TRequestInfoPtr RequestInfo;
        const TVector<TBlockRange64> Ranges;

        TUpdateUsedBlocks(
                TRequestInfoPtr requestInfo,
                TVector<TBlockRange64> ranges)
            : RequestInfo(std::move(requestInfo))
            , Ranges(std::move(ranges))
        {}

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // WriteThrottlerState
    //

    struct TWriteThrottlerState
    {
        const TRequestInfoPtr RequestInfo;
        const TVolumeDatabase::TThrottlerStateInfo StateInfo;

        TWriteThrottlerState(
                TRequestInfoPtr requestInfo,
                TVolumeDatabase::TThrottlerStateInfo stateInfo)
            : RequestInfo(std::move(requestInfo))
            , StateInfo(stateInfo)
        {}

        void Clear()
        {
            // nothing to do
        }
    };
};

}   // namespace NCloud::NBlockStore::NStorage
