#pragma once

#include "public.h"

#include <cloud/filestore/libs/service/public.h>

#include <cloud/storage/core/libs/diagnostics/stats_updater.h>
#include <cloud/storage/core/protos/media.pb.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

struct IServerStats
{
    virtual void RequestStarted(TLog& log, TServerCallContext& callContext) = 0;

    virtual void RequestCompleted(TLog& log, TServerCallContext& callContext) = 0;

    virtual void ResponseSent(TServerCallContext& callContext) = 0;

    virtual void UpdateRestartsCount(int cnt) = 0;

    virtual void UpdateStats(bool updatePercentiles) = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct IRequestStats
{
    virtual void RequestStarted(TLog& log, TCallContext& callContext) = 0;

    virtual void RequestCompleted(TLog& log, TCallContext& callContext) = 0;

    virtual void ResponseSent(TCallContext& callContext) = 0;

    virtual void UpdateStats(bool updatePercentiles) = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct IRequestStatsRegistry
    : public IStats
{
    virtual IServerStatsPtr GetServerStats() = 0;

    virtual IRequestStatsPtr GetFileSystemStats(
        const TString& filesystem,
        const TString& client,
        NCloud::NProto::EStorageMediaKind media) = 0;

    virtual void Unregister(
        const TString& fileSystemId,
        const TString& clientId) = 0;
};

////////////////////////////////////////////////////////////////////////////////

IRequestStatsRegistryPtr CreateRequestStatsRegistry(
    TString component,
    TDiagnosticsConfigPtr diagnosticsConfig,
    NMonitoring::TDynamicCountersPtr rootCounters,
    ITimerPtr timer);

IRequestStatsRegistryPtr CreateRequestStatsRegistryStub();

}   // namespace NCloud::NFileStore
