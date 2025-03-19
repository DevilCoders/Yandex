#pragma once

#include "volume_state.h"

#include <cloud/blockstore/libs/common/compressed_bitmap.h>
#include <cloud/blockstore/libs/storage/protos/volume.pb.h>

#include <ydb/core/tablet_flat/flat_cxx_database.h>

#include <util/generic/maybe.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TVolumeDatabase
    : public NKikimr::NIceDb::TNiceDb
{
public:
    TVolumeDatabase(NKikimr::NTable::TDatabase& database)
        : NKikimr::NIceDb::TNiceDb(database)
    {}

    void InitSchema();

    //
    // Meta
    //

    void WriteMeta(const NProto::TVolumeMeta& meta);
    bool ReadMeta(TMaybe<NProto::TVolumeMeta>& meta);

    //
    // Clients
    //

    void WriteClients(const THashMap<TString, TVolumeClientState>& infos);
    bool ReadClients(THashMap<TString, TVolumeClientState>& infos);

    void WriteClient(const NProto::TVolumeClientInfo& info);
    bool ReadClient(const TString& clientId, TMaybe<NProto::TVolumeClientInfo>& info);

    void RemoveClient(const TString& clientId);

    //
    // History
    //

    void WriteHistory(THistoryLogItem item);

    bool ReadOutdatedHistory(
        TVector<THistoryLogKey>& records,
        TInstant oldestTimestamp);

    bool ReadHistory(
        TVector<THistoryLogItem>& records,
        TInstant startTs,
        TInstant endTs,
        ui64 numRecords);

    bool ReadHistory(
        TDeque<THistoryLogItem>& records,
        TInstant startTs,
        TInstant endTs,
        ui64 numRecords);

    void DeleteHistoryEntry(THistoryLogKey entry);

    //
    // PartStats
    //

    struct TPartStats
    {
        ui64 Id = 0;
        NProto::TCachedPartStats Stats;
    };

    void WritePartStats(
        ui64 partTabletId,
        const NProto::TCachedPartStats& stats);
    bool ReadPartStats(TVector<TPartStats>& stats);

    void WriteNonReplPartStats(ui64 id, const NProto::TCachedPartStats& stats);
    bool ReadNonReplPartStats(TVector<TPartStats>& stats);

    //
    // CheckpointRequests
    //

    void WriteCheckpointRequest(
        ui64 requestId,
        TString checkpointId,
        TInstant timestamp,
        ECheckpointRequestType reqType);

    bool CollectCheckpointsToDelete(
        TDuration deletedCheckpointHistoryLifetime,
        TInstant now,
        THashMap<TString, TInstant>& deletedCheckpoints);
    void MarkCheckpointRequestCompleted(ui64 requestId);
    void MarkCheckpointDeleted(ui64 requestId);
    bool ReadCheckpointRequests(
        const THashMap<TString, TInstant>& deletedCheckpoints,
        TVector<TCheckpointRequest>& requests,
        TVector<ui64>& outdatedCheckpointRequestIds);
    void DeleteCheckpointEntry(ui64 requestId);

    //
    // UsedBlocks
    //

    void WriteUsedBlocks(const TCompressedBitmap::TSerializedChunk& chunk);
    bool ReadUsedBlocks(TCompressedBitmap& usedBlocks);

    //
    // ThrottlerState
    //

    struct TThrottlerStateInfo
    {
        ui64 Budget = 0;
    };

    void WriteThrottlerState(const TThrottlerStateInfo& stateInfo);
    bool ReadThrottlerState(TMaybe<TThrottlerStateInfo>& stateInfo);
};

}   // namespace NCloud::NBlockStore::NStorage
