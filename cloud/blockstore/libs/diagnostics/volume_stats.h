#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/public.h>
#include <cloud/blockstore/libs/service/request.h>
#include <cloud/storage/core/libs/common/error.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

using TVolumePerfStatus = std::pair<TString, ui32>;
using TVolumePerfStatuses = TVector<TVolumePerfStatus>;

////////////////////////////////////////////////////////////////////////////////

enum class EVolumeStatsType
{
    EServerStats,
    EClientStats
};

////////////////////////////////////////////////////////////////////////////////

struct IVolumeInfo
{
    virtual ~IVolumeInfo() = default;

    virtual const NProto::TVolume& GetInfo() const = 0;

    virtual ui64 RequestStarted(
        EBlockStoreRequest requestType,
        ui32 requestBytes) = 0;

    virtual TDuration RequestCompleted(
        EBlockStoreRequest requestType,
        ui64 requestStarted,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind) = 0;

    virtual void AddIncompleteStats(
        EBlockStoreRequest requestType,
        TDuration requestTime) = 0;

    virtual void AddRetryStats(
        EBlockStoreRequest requestType,
        EErrorKind errorKind) = 0;

    virtual void RequestPostponed(EBlockStoreRequest requestType) = 0;
    virtual void RequestAdvanced(EBlockStoreRequest requestType) = 0;
    virtual void RequestFastPathHit(EBlockStoreRequest requestType) = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct IVolumeStats
{
    virtual ~IVolumeStats() = default;

    virtual bool MountVolume(
        const NProto::TVolume& volume,
        const TString& clientId,
        const TString& instanceId) = 0;

    virtual void UnmountVolume(
        const TString& diskId,
        const TString& clientId) = 0;

    virtual IVolumeInfoPtr GetVolumeInfo(
        const TString& diskId,
        const TString& clientId) const = 0;

    virtual ui32 GetBlockSize(const TString& diskId) const = 0;

    virtual void TrimVolumes() = 0;

    virtual void UpdateStats(bool updatePercentiles) = 0;

    virtual TVolumePerfStatuses GatherVolumePerfStatuses() = 0;

    virtual IUserMetricsSupplierPtr GetUserCounters() const { return nullptr; }
};

////////////////////////////////////////////////////////////////////////////////

IVolumeStatsPtr CreateVolumeStats(
    IMonitoringServicePtr monitoring,
    TDiagnosticsConfigPtr diagnosticsConfig,
    TDuration inactiveClientsTimeout,
    EVolumeStatsType type,
    ITimerPtr timer);

IVolumeStatsPtr CreateVolumeStats(
    IMonitoringServicePtr monitoring,
    TDuration inactiveClientsTimeout,
    EVolumeStatsType type,
    ITimerPtr timer);

IVolumeStatsPtr CreateVolumeStatsStub();

}   // namespace NCloud::NBlockStore
