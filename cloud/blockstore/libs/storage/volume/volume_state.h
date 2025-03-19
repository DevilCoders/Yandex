#pragma once

#include "partition_info.h"
#include "public.h"

#include <cloud/blockstore/libs/common/block_range.h>
#include <cloud/blockstore/libs/common/compressed_bitmap.h>
#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/metrics.h>
#include <cloud/blockstore/libs/storage/core/public.h>
#include <cloud/blockstore/libs/storage/partition_nonrepl/public.h>
#include <cloud/blockstore/libs/storage/protos/volume.pb.h>
#include <cloud/blockstore/libs/storage/volume/model/client_state.h>
#include <cloud/blockstore/libs/storage/volume/model/checkpoint.h>
#include <cloud/blockstore/libs/storage/volume/model/volume_throttling_policy.h>
#include <cloud/storage/core/libs/common/error.h>

#include <ydb/core/base/blobstorage.h>

#include <library/cpp/actors/core/actorid.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/size_literals.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

using TDevices = google::protobuf::RepeatedPtrField<NProto::TDeviceConfig>;
using TMigrations = google::protobuf::RepeatedPtrField<NProto::TDeviceMigration>;

////////////////////////////////////////////////////////////////////////////////

struct THistoryLogKey
{
    TInstant Timestamp;
    ui64 Seqno = 0;

    THistoryLogKey() = default;

    THistoryLogKey(TInstant timestamp, ui64 seqno = 0)
        : Timestamp(timestamp)
        , Seqno(seqno)
    {}

    bool operator == (const THistoryLogKey& rhs) const;
    bool operator != (const THistoryLogKey& rhs) const;
    bool operator < (THistoryLogKey rhs) const;
};

struct THistoryLogItem
{
    THistoryLogKey Key;
    NProto::TVolumeOperation Operation;

    THistoryLogItem() = default;

    THistoryLogItem(
            THistoryLogKey key,
            NProto::TVolumeOperation operation)
        : Key(key)
        , Operation(std::move(operation))
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct TPartitionStatInfo
{
    NActors::TActorId Owner = {};
    TPartitionDiskCountersPtr LastCounters = {};
    TPartitionDiskCounters CachedCounters;
    ui64 LastSystemCpu = 0;
    ui64 LastUserCpu = 0;
    NBlobMetrics::TBlobLoadMetrics LastMetrics;

    TPartitionStatInfo(EPublishingPolicy countersPolicy)
        : CachedCounters(countersPolicy)
    {}
};

////////////////////////////////////////////////////////////////////////////////

ui64 ComputeBlockCount(const NProto::TVolumeMeta& meta);

////////////////////////////////////////////////////////////////////////////////

class TVolumeState
{
private:
    TStorageConfigPtr StorageConfig;
    NProto::TVolumeMeta Meta;
    const NProto::TPartitionConfig* Config;
    ui64 BlockCount = 0;

    TPartitionInfoList Partitions;
    TPartitionInfo::EState PartitionsState = TPartitionInfo::UNKNOWN;
    NActors::TActorId NonreplicatedPartitionActor;
    TNonreplicatedPartitionConfigPtr NonreplicatedPartitionConfig;

    TVector<TPartitionStatInfo> PartitionStatInfos;

    THashMap<TString, TVolumeClientState> ClientInfosByClientId;
    TString ReadWriteAccessClientId;
    TString LocalMountClientId;

    THashMultiMap<NActors::TActorId, TString> ClientIdsByPipeServerId;

    TThrottlerConfig ThrottlerConfig;
    TVolumeThrottlingPolicy ThrottlingPolicy;

    ui64 MountSeqNumber = 0;

    EStorageAccessMode StorageAccessMode = EStorageAccessMode::Default;
    bool ForceRepair = false;
    bool AcceptInvalidDiskAllocationResponse = false;
    bool RejectWrite = false;

    THistoryLogKey LastLogRecord;
    TDeque<THistoryLogItem> History;

    ui64 LastCheckpointRequestId = 0;
    TVector<TCheckpointRequest> CheckpointRequests;
    TCheckpointStore ActiveCheckpoints;

    std::unique_ptr<TCompressedBitmap> UsedBlocks;
    bool TrackUsedBlocks = false;
    bool MaskUnusedBlocks = false;
    bool UseRdma = false;
    bool RdmaUnavailable = false;

public:
    TVolumeState(
        TStorageConfigPtr storageConfig,
        NProto::TVolumeMeta meta,
        TThrottlerConfig throttlerConfig,
        THashMap<TString, TVolumeClientState> infos,
        TDeque<THistoryLogItem> history,
        TVector<TCheckpointRequest> checkpointRequests);

    const NProto::TVolumeMeta& GetMeta() const
    {
        return Meta;
    }

    void UpdateMigrationIndexInMeta(ui64 migrationIndex)
    {
        Meta.SetMigrationIndex(migrationIndex);
    }

    void ResetMeta(NProto::TVolumeMeta meta);
    void Reset();

    //
    // Config
    //

    const NProto::TPartitionConfig& GetConfig() const
    {
        return *Config;
    }

    const TString& GetDiskId() const
    {
        return Config->GetDiskId();
    }

    const TString& GetBaseDiskId() const
    {
        return Config->GetBaseDiskId();
    }

    const TString& GetBaseDiskCheckpointId() const
    {
        return Config->GetBaseDiskCheckpointId();
    }

    ui32 GetBlockSize() const
    {
        return Config->GetBlockSize();
    }

    ui64 GetBlocksCount() const
    {
        return BlockCount;
    }

    void FillDeviceInfo(NProto::TVolume& volume) const;

    //
    // Partitions
    //

    TPartitionInfoList& GetPartitions()
    {
        return Partitions;
    }

    TPartitionInfo* GetPartition(ui64 tabletId);
    bool FindPartitionIndex(ui64 tabletId, ui32& index) const;

    //
    // State
    //

    TPartitionInfo::EState GetPartitionsState() const
    {
        return PartitionsState;
    }

    bool Ready()
    {
        return UpdatePartitionsState() == TPartitionInfo::READY;
    }

    void SetPartitionsState(TPartitionInfo::EState state);
    TPartitionInfo::EState UpdatePartitionsState();

    TString GetPartitionsError() const;

    void SetNonreplicatedPartitionActor(
        const NActors::TActorId& actor,
        TNonreplicatedPartitionConfigPtr config)
    {
        PartitionStatInfos[0].Owner = actor;
        NonreplicatedPartitionActor = actor;
        NonreplicatedPartitionConfig = std::move(config);
    }

    const NActors::TActorId& GetNonreplicatedPartitionActor() const
    {
        return NonreplicatedPartitionActor;
    }

    const TNonreplicatedPartitionConfigPtr& GetNonreplicatedPartitionConfig() const
    {
        return NonreplicatedPartitionConfig;
    }

    bool FindPartitionStatInfoByOwner(const NActors::TActorId& actorId, ui32& index) const;

    TPartitionStatInfo* GetPartitionStatInfoById(ui64 id);

    bool SetPartitionStatActor(ui64 id, const NActors::TActorId& actor);

    const TVector<TPartitionStatInfo>& GetPartitionStatInfos() const
    {
        return PartitionStatInfos;
    }

    TVector<TPartitionStatInfo>& GetPartitionStatInfos()
    {
        return PartitionStatInfos;
    }

    EStorageAccessMode GetStorageAccessMode() const
    {
        return ForceRepair ? EStorageAccessMode::Repair : StorageAccessMode;
    }

    bool GetRejectWrite() const
    {
        return RejectWrite;
    }

    bool GetAcceptInvalidDiskAllocationResponse() const
    {
        return AcceptInvalidDiskAllocationResponse;
    }

    bool GetMuteIOErrors() const;

    //
    // Clients
    //

    struct TAddClientResult
    {
        NProto::TError Error;
        TVector<TString> RemovedClientIds;

        TAddClientResult() = default;

        TAddClientResult(NProto::TError error)
            : Error(std::move(error))
        {}
    };

    TAddClientResult AddClient(
        const NProto::TVolumeClientInfo& info,
        const NActors::TActorId& pipeServerActorId = {},
        const NActors::TActorId& SenderActorId = {},
        TInstant referenceTimestamp = TInstant::Now());

    bool IsClientStale(
        const TString& clientId,
        TInstant referenceTimestamp = TInstant::Now()) const;

    bool IsClientStale(
        const TVolumeClientState& clientInfo,
        TInstant referenceTimestamp = TInstant::Now()) const;

    const NProto::TVolumeClientInfo* GetClient(const TString& clientId) const;

    NProto::TError RemoveClient(
        const TString& clientId,
        const NActors::TActorId& pipeServerActorId);

    bool HasClients() const
    {
        return !ClientInfosByClientId.empty();
    }

    bool HasActiveClients(TInstant referenceTimestamp) const;
    bool IsPreempted(NActors::TActorId selfId) const;

    const THashMap<TString, TVolumeClientState>& GetClients() const
    {
        return ClientInfosByClientId;
    }

    THashMap<TString, TVolumeClientState>& AccessClients()
    {
        return ClientInfosByClientId;
    }

    TString GetReadWriteAccessClientId() const
    {
        return ReadWriteAccessClientId;
    }

    TString GetLocalMountClientId() const
    {
        return LocalMountClientId;
    }

    //
    // Connected services
    //

    void SetServiceDisconnected(
        const NActors::TActorId& pipeServerActorId,
        TInstant disconnectTime);

    void UnmapClientFromPipeServerId(
        const NActors::TActorId& pipeServerActorId,
        const TString& clientId);

    const THashMultiMap<NActors::TActorId, TString>& GetPipeServerId2ClientId() const;

    TVector<NActors::TActorId> ClearPipeServerIds();

    //
    // Throttling
    //

    TVolumeThrottlingPolicy& AccessThrottlingPolicy()
    {
        return ThrottlingPolicy;
    }

    const TVolumeThrottlingPolicy& GetThrottlingPolicy() const
    {
        return ThrottlingPolicy;
    }

    //
    // MountSeqNumber
    //

    ui64 GetMountSeqNumber() const
    {
        return MountSeqNumber;
    }

    void SetMountSeqNumber(ui64 mountSeqNumber)
    {
        MountSeqNumber = mountSeqNumber;
    }

    //
    // Volume operation log
    //

    THistoryLogItem LogAddClient(
        TInstant timestamp,
        const NProto::TVolumeClientInfo& add,
        const NProto::TError& error,
        const NActors::TActorId& pipeServer,
        const NActors::TActorId& senderId);
    THistoryLogItem LogRemoveClient(
        TInstant timestamp,
        const TString& clientId,
        const TString& reason,
        const NProto::TError& error);
    THistoryLogItem LogUpdateMeta(
        TInstant timestamp,
        const NProto::TVolumeMeta& meta);

    const TDeque<THistoryLogItem>& GetHistory() const
    {
        return History;
    }

    THistoryLogKey GetRecentLogEvent() const
    {
        if (History.size()) {
            return History.front().Key;
        } else {
            return {};
        }
    }

    void CleanupHistoryIfNeeded(TInstant oldest);

    //
    // Checkpoint request history
    //

    const TVector<TCheckpointRequest>& GetCheckpointRequests() const
    {
        return CheckpointRequests;
    }

    TVector<TCheckpointRequest>& GetCheckpointRequests()
    {
        return CheckpointRequests;
    }

    ui64 GenerateCheckpointRequestId()
    {
        return ++LastCheckpointRequestId;
    }

    TCheckpointStore& GetActiveCheckpoints()
    {
        return ActiveCheckpoints;
    }

    const TCheckpointStore& GetActiveCheckpoints() const
    {
        return ActiveCheckpoints;
    }

    //
    // UsedBlocks
    //

    bool GetTrackUsedBlocks() const
    {
        return TrackUsedBlocks;
    }

    bool GetMaskUnusedBlocks() const
    {
        return MaskUnusedBlocks;
    }

    bool GetUseRdma() const
    {
        return UseRdma && !RdmaUnavailable;
    }

    void SetRdmaUnavailable()
    {
        RdmaUnavailable = true;
    }

    const TCompressedBitmap* GetUsedBlocks() const
    {
        return UsedBlocks.get();
    }

    TCompressedBitmap& AccessUsedBlocks()
    {
        if (!UsedBlocks) {
            Y_VERIFY(BlockCount);
            UsedBlocks = std::make_unique<TCompressedBitmap>(BlockCount);
        }

        return *UsedBlocks;
    }

private:
    bool CanPreemptClient(
        const TString& oldClientId,
        TInstant referenceTimestamp,
        ui64 clientMountSeqNumber);

    THistoryLogKey AllocateHistoryLogKey(TInstant timestamp);
};

}   // namespace NCloud::NBlockStore::NStorage
