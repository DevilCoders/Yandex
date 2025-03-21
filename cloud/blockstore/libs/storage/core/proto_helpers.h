#pragma once

#include "public.h"

#include <cloud/blockstore/libs/kikimr/events.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <util/generic/string.h>
#include <util/generic/typetraits.h>

namespace NCloud::NBlockStore::NStorage {

namespace NImpl {

////////////////////////////////////////////////////////////////////////////////

Y_HAS_MEMBER(GetSessionId);

template <typename T, typename = void>
struct TGetSessionIdSelector;

template <typename T>
struct TGetSessionIdSelector<T, std::enable_if_t<THasGetSessionId<T>::value>>
{
    static TString Get(const T& args)
    {
        return args.GetSessionId();
    }
};

template <typename T>
struct TGetSessionIdSelector<T, std::enable_if_t<!THasGetSessionId<T>::value>>
{
    static TString Get(const T& args)
    {
        Y_UNUSED(args);
        return {};
    }
};

template <typename T>
using TGetSessionId = TGetSessionIdSelector<T>;

}   // NImpl

////////////////////////////////////////////////////////////////////////////////

template <typename TArgs, ui32 EventId>
TString GetIdempotenceId(const TRequestEvent<TArgs, EventId>& request)
{
    return request.Headers.GetIdempotenceId();
}

template <typename TArgs, ui32 EventId>
TString GetIdempotenceId(const TProtoRequestEvent<TArgs, EventId>& request)
{
    return request.Record.GetHeaders().GetIdempotenceId();
}

template <typename TArgs, ui32 EventId>
TString GetClientId(const TRequestEvent<TArgs, EventId>& request)
{
    return request.Headers.GetClientId();
}

template <typename TArgs, ui32 EventId>
TString GetClientId(const TProtoRequestEvent<TArgs, EventId>& request)
{
    return request.Record.GetHeaders().GetClientId();
}

template <typename TArgs, ui32 EventId>
TString GetDiskId(const TRequestEvent<TArgs, EventId>& request)
{
    return request.DiskId;
}

template <typename TArgs, ui32 EventId>
TString GetDiskId(const TProtoRequestEvent<TArgs, EventId>& request)
{
    return request.Record.GetDiskId();
}

template <typename TArgs, ui32 EventId>
TString GetSessionId(const TRequestEvent<TArgs, EventId>& request)
{
    return request.SessionId;
}

template <typename TArgs, ui32 EventId>
TString GetSessionId(const TProtoRequestEvent<TArgs, EventId>& request)
{
    return NImpl::TGetSessionId<TArgs>::Get(request.Record);
}

void VolumeConfigToVolume(
    const NKikimrBlockStore::TVolumeConfig& volumeConfig,
    NProto::TVolume& volume);

void VolumeConfigToVolumeModel(
    const NKikimrBlockStore::TVolumeConfig& volumeConfig,
    NProto::TVolumeModel& volumeModel);

void FillDeviceInfo(
    const google::protobuf::RepeatedPtrField<NProto::TDeviceConfig>& devices,
    const google::protobuf::RepeatedPtrField<NProto::TDeviceMigration>& migrations,
    const google::protobuf::RepeatedPtrField<NProto::TReplica>& replicas,
    NProto::TVolume& volume);

template <typename TArgs, ui32 EventId>
ui32 GetRequestGeneration(const TProtoRequestEvent<TArgs, EventId>& request)
{
    return request.Record.GetHeaders().GetRequestGeneration();
}

template <typename TArgs, ui32 EventId>
ui32 GetRequestGeneration(const TRequestEvent<TArgs, EventId>& request)
{
    return request.Headers.GetRequestGeneration();
}

template <typename TArgs, ui32 EventId>
void SetRequestGeneration(
    ui32 generation,
    TProtoRequestEvent<TArgs, EventId>& request)
{
    request.Record.MutableHeaders()->SetRequestGeneration(generation);
}

template <typename TArgs, ui32 EventId>
void SetRequestGeneration(
    ui32 generation,
    TRequestEvent<TArgs, EventId>& request)
{
    request.Headers.SetRequestGeneration(generation);
}

bool GetThrottlingEnabled(
    const TStorageConfig& config,
    const NProto::TPartitionConfig& partitionConfig);

bool CompareVolumeConfigs(
    const NKikimrBlockStore::TVolumeConfig& prevConfig,
    const NKikimrBlockStore::TVolumeConfig& newConfig);

ui32 GetWriteBlobThreshold(
    const TStorageConfig& config,
    const NCloud::NProto::EStorageMediaKind mediaKind);

inline bool RequiresCheckpointSupport(const NProto::TReadBlocksRequest& request)
{
    return !request.GetCheckpointId().empty();
}

inline bool RequiresCheckpointSupport(const NProto::TWriteBlocksRequest&)
{
    return false;
}

inline bool RequiresCheckpointSupport(const NProto::TZeroBlocksRequest&)
{
    return false;
}

}   // namespace NCloud::NBlockStore::NStorage
