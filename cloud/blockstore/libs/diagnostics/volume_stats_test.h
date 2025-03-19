#pragma once

#include "public.h"

#include "volume_stats.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/set.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

struct TDefaultRequestProcessingPolicy
{
    void RequestPostponed(EBlockStoreRequest requestType)
    {
        Y_UNUSED(requestType);
    }

    void RequestAdvanced(EBlockStoreRequest requestType)
    {
        Y_UNUSED(requestType);
    }
};

template <typename TRequestProcessingPolicy = TDefaultRequestProcessingPolicy>
class TTestVolumeInfo final
    : public IVolumeInfo
    , public TRequestProcessingPolicy
{
public:
    NProto::TVolume Volume;

    const NProto::TVolume& GetInfo() const override
    {
        return Volume;
    }

    ui64 RequestStarted(
        EBlockStoreRequest requestType,
        ui32 requestBytes) override
    {
        Y_UNUSED(requestType);
        Y_UNUSED(requestBytes);
        return 0;
    }

    TDuration RequestCompleted(
        EBlockStoreRequest requestType,
        ui64 requestStarted,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind) override
    {
        Y_UNUSED(requestType);
        Y_UNUSED(requestStarted);
        Y_UNUSED(postponedTime);
        Y_UNUSED(requestBytes);
        Y_UNUSED(errorKind);
        return TDuration::Zero();
    }

    void AddIncompleteStats(
        EBlockStoreRequest requestType,
        TDuration requestTime) override
    {
        Y_UNUSED(requestType);
        Y_UNUSED(requestTime);
    }

    void AddRetryStats(
        EBlockStoreRequest requestType,
        EErrorKind errorKind) override
    {
        Y_UNUSED(requestType);
        Y_UNUSED(errorKind);
    }

    void RequestPostponed(EBlockStoreRequest requestType) override
    {
        TRequestProcessingPolicy::RequestPostponed(requestType);
    }

    void RequestAdvanced(EBlockStoreRequest requestType) override
    {
        TRequestProcessingPolicy::RequestAdvanced(requestType);
    }

    void RequestFastPathHit(EBlockStoreRequest requestType) override
    {
        Y_UNUSED(requestType);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TDefaultVolumeProcessingPolicy
{
    TSet<TString> DiskIds;

    bool MountVolume(
        const NProto::TVolume& volume,
        const TString& clientId,
        const TString& instanceId)
    {
        Y_UNUSED(clientId);
        Y_UNUSED(instanceId);

        UNIT_ASSERT(!DiskIds.contains(volume.GetDiskId()));

        DiskIds.insert(volume.GetDiskId());
        return false;
    }

    void UnmountVolume(
        const TString& diskId,
        const TString& clientId)
    {
        Y_UNUSED(clientId);

        auto it = DiskIds.find(diskId);
        UNIT_ASSERT(it != DiskIds.end());

        DiskIds.erase(it);
    }

    IVolumeInfoPtr GetVolumeInfo(
        const TString& diskId,
        const TString& clientId) const
    {
        Y_UNUSED(diskId);
        Y_UNUSED(clientId);

        return nullptr;
    }
};

template <typename TVolumeProcessingPolicy = TDefaultVolumeProcessingPolicy>
class TTestVolumeStats final
    : public IVolumeStats
    , public TVolumeProcessingPolicy
{
public:
    bool MountVolume(
        const NProto::TVolume& volume,
        const TString& clientId,
        const TString& instanceId) override
    {
        return TVolumeProcessingPolicy::MountVolume(
            volume,
            clientId,
            instanceId);
    }

    void UnmountVolume(
        const TString& diskId,
        const TString& clientId) override
    {
        TVolumeProcessingPolicy::UnmountVolume(diskId, clientId);
    }

    IVolumeInfoPtr GetVolumeInfo(
        const TString& diskId,
        const TString& clientId) const override
    {
        return TVolumeProcessingPolicy::GetVolumeInfo(diskId, clientId);
    }

    ui32 GetBlockSize(const TString& diskId) const override
    {
        Y_UNUSED(diskId);

        return 0;
    }

    void UpdateStats(bool updatePercentiles) override
    {
        Y_UNUSED(updatePercentiles);
    }

    void TrimVolumes() override
    {}

    TVolumePerfStatuses GatherVolumePerfStatuses()  override
    {
        return {};
    }
};

}   // namespace NCloud::NBlockStore
