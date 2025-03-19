#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/public.h>
#include <cloud/blockstore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/request_counters.h>

#include <util/datetime/base.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

struct IRequestStats
{
    virtual ~IRequestStats() = default;

    virtual ui64 RequestStarted(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        ui32 requestBytes) = 0;

    virtual TDuration RequestCompleted(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        ui64 requestStarted,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind) = 0;

    virtual void AddIncompleteStats(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        TDuration requestTime) = 0;

    virtual void AddRetryStats(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        EErrorKind errorKind) = 0;

    virtual void RequestPostponed(EBlockStoreRequest requestType) = 0;
    virtual void RequestAdvanced(EBlockStoreRequest requestType) = 0;
    virtual void RequestFastPathHit(EBlockStoreRequest requestType) = 0;

    virtual void UpdateStats(bool updatePercentiles) = 0;
};

////////////////////////////////////////////////////////////////////////////////

IRequestStatsPtr CreateClientRequestStats(
    NMonitoring::TDynamicCountersPtr counters,
    ITimerPtr timer);
IRequestStatsPtr CreateServerRequestStats(
    NMonitoring::TDynamicCountersPtr counters,
    ITimerPtr timer);
IRequestStatsPtr CreateRequestStatsStub();

}   // namespace NCloud::NBlockStore
