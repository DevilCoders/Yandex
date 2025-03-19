#include "service_state.h"

#include <cloud/blockstore/libs/common/proto_helpers.h>

#include <cloud/storage/core/libs/common/media.h>

#include <util/generic/guid.h>
#include <util/generic/hash_set.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

bool ShouldChangePreemptionType(
    NProto::EPreemptionSource current,
    NProto::EPreemptionSource candidate)
{
    switch (current) {
        case NProto::SOURCE_INITIAL_MOUNT: {
            return false;
        }
        case NProto::SOURCE_MANUAL: {
            return candidate == NProto::SOURCE_INITIAL_MOUNT;
        }
        case NProto::SOURCE_BALANCER: {
            return candidate != NProto::SOURCE_NONE;
        }
        case NProto::SOURCE_NONE: {
            return true;
        }
        default: {
            Y_VERIFY(0);
        }
    }
}

}   // namespace


////////////////////////////////////////////////////////////////////////////////

TVolumeInfo::TVolumeInfo(
        TString diskId)
    : DiskId(std::move(diskId))
    , SessionId(CreateGuidAsString())
{}

TClientInfo* TVolumeInfo::GetClientInfo(const TString& clientId) const
{
    auto it = ClientInfosByClientId.find(clientId);
    if (it != ClientInfosByClientId.end()) {
        return it->second;
    }

    return nullptr;
}

TClientInfo* TVolumeInfo::GetReadWriteAccessClientInfo() const
{
    for (const auto& pair: ClientInfosByClientId) {
        if (IsReadWriteMode(pair.second->VolumeAccessMode)) {
            return pair.second;
        }
    }

    return nullptr;
}

TClientInfo* TVolumeInfo::GetLocalMountClientInfo() const
{
    for (const auto& pair: ClientInfosByClientId) {
        if (pair.second->VolumeMountMode == NProto::VOLUME_MOUNT_LOCAL) {
            return pair.second;
        }
    }

    return nullptr;
}

TClientInfo* TVolumeInfo::AddClientInfo(const TString& clientId)
{
    {
        TClientInfo* info = GetClientInfo(clientId);
        if (info) {
            return info;
        }
    }

    auto info = std::make_unique<TClientInfo>(
        clientId,
        DiskId);

    ClientInfos.PushBack(info.get());
    ClientInfosByClientId.emplace(clientId, info.get());
    return info.release();
}

void TVolumeInfo::RemoveClientInfo(TClientInfo* info)
{
    std::unique_ptr<TClientInfo> deleter(info);
    ClientInfosByClientId.erase(info->ClientId);
    info->Unlink();
}

void TVolumeInfo::RemoveClientInfo(const TString& clientId)
{
    if (auto it = ClientInfosByClientId.find(clientId); it != ClientInfosByClientId.end()) {
        auto* info = it->second;
        std::unique_ptr<TClientInfo> deleter(info);
        ClientInfosByClientId.erase(info->ClientId);
        info->Unlink();
    }
}

bool TVolumeInfo::IsMounted() const
{
    return !ClientInfos.Empty();
}

bool TVolumeInfo::IsReadWriteMounted() const
{
    for (const auto& info: ClientInfos) {
        if (IsReadWriteMode(info.VolumeAccessMode)) {
            return true;
        }
    }

    return false;
}

bool TVolumeInfo::IsLocallyMounted() const
{
    for (const auto& info: ClientInfos) {
        if (info.VolumeMountMode == NProto::VOLUME_MOUNT_LOCAL) {
            return true;
        }
    }

    return false;
}

bool TVolumeInfo::IsDiskRegistryVolume() const
{
    if (VolumeInfo) {
        return IsDiskRegistryMediaKind(VolumeInfo->GetStorageMediaKind());
    }

    return false;
}

void TVolumeInfo::SetStarted(
    ui64 tabletId,
    const NProto::TVolume& volumeInfo,
    const TActorId& volumeActor)
{
    TabletId = tabletId;
    VolumeInfo = volumeInfo;
    VolumeActor = volumeActor;

    State = STARTED;
    Error = {};
}

void TVolumeInfo::SetFailed(const NProto::TError& error)
{
    State = FAILED;
    VolumeActor = {};
    Error = error;
}

TString TVolumeInfo::GetStatus() const
{
    TStringStream out;

    switch (State) {
        default:
        case INITIAL:
            out << "INITIAL";
            break;
        case STARTED:
            out << "STARTED";
            break;
        case FAILED:
            out << "FAILED";
            break;
    }

    if (FAILED(Error.GetCode())) {
        out << ": " << FormatError(Error);
    }

    return out.Str();
}

////////////////////////////////////////////////////////////////////////////////

NProto::EVolumeBinding TVolumeInfo::OnMountStarted(
    TSharedServiceCounters& sharedCounters,
    NProto::EPreemptionSource preemptionSource,
    NProto::EVolumeBinding explicitBindingType,
    NProto::EVolumeMountMode clientMode)
{
    auto bindingType = CalcVolumeBinding(preemptionSource, explicitBindingType, clientMode);

    if (bindingType != BindingType &&
        bindingType == NProto::BINDING_LOCAL &&
        !SharedCountersLockAcquired)
    {
        if (!sharedCounters.TryAcquireLocalVolume()) {
            bindingType = NProto::BINDING_REMOTE;
            PreemptionSource = NProto::SOURCE_INITIAL_MOUNT;
        } else {
            SharedCountersLockAcquired = true;
        }
    }

    return bindingType;
}

void TVolumeInfo::OnMountCancelled(TSharedServiceCounters& sharedCounters)
{
    if (SharedCountersLockAcquired && !IsLocallyMounted()) {
        sharedCounters.ReleaseLocalVolume();
        SharedCountersLockAcquired = false;
    }
}

void TVolumeInfo::OnMountFinished(
    TSharedServiceCounters& sharedCounters,
    NProto::EPreemptionSource preemptionSource,
    NProto::EVolumeBinding bindingType,
    const NProto::TError& error)
{
    if (SUCCEEDED(error.GetCode())) {
        BindingType = bindingType;
        if (bindingType == NProto::BINDING_LOCAL) {
            PreemptionSource = NProto::SOURCE_NONE;
        } else if (preemptionSource != NProto::SOURCE_NONE) {
            if (ShouldChangePreemptionType(PreemptionSource, preemptionSource)) {
                PreemptionSource = preemptionSource;
            }
        }
    }

    if (!IsLocallyMounted() && SharedCountersLockAcquired) {
        sharedCounters.ReleaseLocalVolume();
        SharedCountersLockAcquired = false;
        BindingType = NProto::BINDING_REMOTE;
    }
}

void TVolumeInfo::OnClientRemoved(
    TSharedServiceCounters& sharedCounters)
{
    if (!IsLocallyMounted() && SharedCountersLockAcquired) {
        sharedCounters.ReleaseLocalVolume();
        SharedCountersLockAcquired = false;
        BindingType = NProto::BINDING_REMOTE;
        PreemptionSource = NProto::SOURCE_NONE;
    }
}

NProto::EVolumeBinding TVolumeInfo::CalcVolumeBinding(
    NProto::EPreemptionSource preemptionSource,
    NProto::EVolumeBinding explicitBindingType,
    NProto::EVolumeMountMode clientMode) const
{
    if (clientMode != NProto::VOLUME_MOUNT_LOCAL) {
        return BindingType;
    }

    // check if request came from monitoring or balancer
    if (preemptionSource != NProto::SOURCE_NONE) {
        if (preemptionSource == PreemptionSource) {
            return explicitBindingType;
        }
        if (!ShouldChangePreemptionType(PreemptionSource, preemptionSource)) {
            return BindingType;
        } else {
            return explicitBindingType;
        }
    }

    // if volume is preempted then generic mount
    // should not change binding
    if (PreemptionSource == NProto::SOURCE_MANUAL ||
        PreemptionSource == NProto::SOURCE_BALANCER)
    {
        return BindingType;
    }

    const bool isLocallyMounted = IsLocallyMounted();
    const bool alreadyRunsLocally = isLocallyMounted && BindingType != NProto::BINDING_REMOTE;
    const bool isMountIncomplete = PreemptionSource == NProto::SOURCE_INITIAL_MOUNT;
    const bool needsLocalMount = !isLocallyMounted || isMountIncomplete;

    if (alreadyRunsLocally || !needsLocalMount) {
        return BindingType;
    }

    return NProto::BINDING_LOCAL;
}

////////////////////////////////////////////////////////////////////////////////

TClientInfo::TClientInfo(
      TString clientId,
      TString diskId)
    : ClientId(std::move(clientId))
    , DiskId(std::move(diskId))
{}

////////////////////////////////////////////////////////////////////////////////

void TServiceState::RemoveVolume(TVolumeInfoPtr volume)
{
    VolumesById.erase(volume->DiskId);
}

TVolumeInfoPtr TServiceState::GetVolume(const TString& diskId) const
{
    auto it = VolumesById.find(diskId);
    return it != VolumesById.end() ? it->second : nullptr;
}

TVolumeInfoPtr TServiceState::GetOrAddVolume(const TString& diskId)
{
    TVolumesMap::insert_ctx ctx;
    auto it = VolumesById.find(diskId, ctx);
    if (it == VolumesById.end()) {
        auto volume = std::make_shared<TVolumeInfo>(diskId);
        it = VolumesById.emplace_direct(ctx, diskId, std::move(volume));
    }

    return it->second;
}

////////////////////////////////////////////////////////////////////////////////

bool TSharedServiceCounters::TryAcquireLocalVolume()
{
    for(;;) {
        auto val = AtomicGet(LocalVolumeCount);
        if (val >= Config->GetMaxLocalVolumes()) {
            return false;
        }
        if (AtomicCas(&LocalVolumeCount, val+1, val)) {
            return true;
        }
    }
}

void TSharedServiceCounters::ReleaseLocalVolume()
{
    Y_VERIFY_DEBUG(AtomicGet(LocalVolumeCount));
    AtomicDecrement(LocalVolumeCount);
}

}   // namespace NCloud::NBlockStore::NStorage
