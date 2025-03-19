#pragma once

#include "public.h"

#include "config.h"

#include <cloud/blockstore/libs/storage/core/public.h>

#include <library/cpp/actors/core/actorid.h>

#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TReplicaInfo
{
    TNonreplicatedPartitionConfigPtr Config;
    google::protobuf::RepeatedPtrField<NProto::TDeviceMigration> Migrations;
};

class TMirrorPartitionState
{
private:
    const TStorageConfigPtr Config;
    TString RWClientId;

    TVector<TReplicaInfo> ReplicaInfos;
    TVector<NActors::TActorId> ReplicaActors;

    ui32 ReadReplicaIndex = 0;

    bool MigrationConfigPrepared = false;

public:
    TMirrorPartitionState(
        TStorageConfigPtr config,
        TString rwClientId,
        TNonreplicatedPartitionConfigPtr partConfig,
        TVector<TDevices> replicas);

public:
    const TVector<TReplicaInfo>& GetReplicaInfos() const
    {
        return ReplicaInfos;
    }

    void AddReplicaActor(const NActors::TActorId& actorId)
    {
        ReplicaActors.push_back(actorId);
    }

    const TVector<NActors::TActorId>& GetReplicaActors() const
    {
        return ReplicaActors;
    }

    void SetRWClientId(TString rwClientId)
    {
        RWClientId = std::move(rwClientId);
    }

    const TString& GetRWClientId() const
    {
        return RWClientId;
    }

    [[nodiscard]] NProto::TError Validate();
    [[nodiscard]] NProto::TError PrepareMigrationConfig();

    [[nodiscard]] NProto::TError NextReadReplica(
        const TBlockRange64 readRange,
        NActors::TActorId* actorId);
};

}   // namespace NCloud::NBlockStore::NStorage
