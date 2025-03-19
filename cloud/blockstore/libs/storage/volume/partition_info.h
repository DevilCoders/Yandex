#pragma once

#include <cloud/blockstore/libs/storage/protos/part.pb.h>

#include <cloud/storage/core/libs/kikimr/public.h>

#include <ydb/core/base/blobstorage.h>

#include <library/cpp/actors/core/actorid.h>
#include <library/cpp/actors/core/scheduler_cookie.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TPartitionInfo
{
    enum EState
    {
        UNKNOWN,
        STOPPED,
        STARTED,
        READY,
        FAILED,
    };

    const ui64 TabletId;
    const NProto::TPartitionConfig PartitionConfig;

    TDuration RetryTimeout;
    NActors::TSchedulerCookieHolder RetryCookie;

    bool RequestingBootExternal = false;
    ui32 SuggestedGeneration = 0;
    NKikimr::TTabletStorageInfoPtr StorageInfo;

    NActors::TActorId Bootstrapper;
    NActors::TActorId Owner;

    EState State = UNKNOWN;
    TString Message;

    TPartitionInfo(ui64 tabletId, NProto::TPartitionConfig partitionConfig)
        : TabletId(tabletId)
        , PartitionConfig(std::move(partitionConfig))
    {}

    void Init(const NActors::TActorId& bootstrapper)
    {
        Bootstrapper = bootstrapper;
        State = UNKNOWN;
        Message = {};
    }

    void SetStarted(const NActors::TActorId& owner)
    {
        Owner = owner;
        State = STARTED;
        Message = {};
    }

    void SetReady()
    {
        Y_VERIFY(State == STARTED);
        State = READY;
    }

    void SetStopped()
    {
        Owner = {};
        State = STOPPED;
        Message = {};
    }

    void SetFailed(TString message)
    {
        Owner = {};
        State = FAILED;
        Message = std::move(message);
    }

    TString GetStatus() const;
};

using TPartitionInfoList = TDeque<TPartitionInfo>;

}   // namespace NCloud::NBlockStore::NStorage
