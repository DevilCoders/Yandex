#pragma once

#include "public.h"

#include "server_stats.h"

namespace NCloud::NBlockStore {

using namespace std::placeholders;

////////////////////////////////////////////////////////////////////////////////

class TTestServerStats final
    : public IServerStats
{
private:
    IServerStatsPtr Stub = CreateServerStatsStub();

public:
    std::function<NMonitoring::TDynamicCounters::TCounterPtr(
        NProto::EClientIpcType ipcType)> GetEndpointCounterHandler
            = std::bind(&IServerStats::GetEndpointCounter, Stub.get(), _1);

    std::function<bool(
        const NProto::TVolume& volume,
        const TString& clientId,
        const TString& instanceId)> MountVolumeHandler
            = std::bind(&IServerStats::MountVolume, Stub.get(), _1, _2, _3);

    std::function<void(
        const TString& diskId,
        const TString& clientId)> UnmountVolumeHandler
            = std::bind(&IServerStats::UnmountVolume, Stub.get(), _1, _2);

    std::function<ui32(const TString& diskId)> GetBlockSizeHandler
        = std::bind(&IServerStats::GetBlockSize, Stub.get(), _1);

    std::function<void(
        TMetricRequest& metricRequest,
        TString clientId,
        TString diskId,
        ui64 startIndex,
        ui32 requestBytes)> PrepareMetricRequestHandler
            = std::bind(&IServerStats::PrepareMetricRequest, Stub.get(), _1, _2, _3, _4, _5);

    std::function<void(
        TLog& log,
        TMetricRequest& metricRequest,
        TCallContext& callContext,
        const TString& message)> RequestStartedHandler
            = std::bind(&IServerStats::RequestStarted, Stub.get(), _1, _2, _3, _4);

    std::function<void(
        TMetricRequest& metricRequest,
        TCallContext& callContext)> RequestAcquiredHandler
            = std::bind(&IServerStats::RequestAcquired, Stub.get(), _1, _2);

    std::function<void(
        TMetricRequest& metricRequest,
        TCallContext& callContext)> RequestSentHandler
            = std::bind(&IServerStats::RequestSent, Stub.get(), _1, _2);

    std::function<void(
        TMetricRequest& metricRequest,
        TCallContext& callContext)> ResponseReceivedHandler
            = std::bind(&IServerStats::ResponseReceived, Stub.get(), _1, _2);

    std::function<void(
        TMetricRequest& metricRequest,
        TCallContext& callContext)> ResponseSentHandler
            = std::bind(&IServerStats::ResponseSent, Stub.get(), _1, _2);

    std::function<void(
        TLog& log,
        TMetricRequest& metricRequest,
        TCallContext& callContext,
        const NProto::TError& error)> RequestCompletedHandler
            = std::bind(&IServerStats::RequestCompleted, Stub.get(), _1, _2, _3, _4);

    std::function<void(
        const TString& diskId,
        const TString& clientId,
        EBlockStoreRequest requestType)> RequestFastPathHitHandler
            = std::bind(&IServerStats::RequestFastPathHit, Stub.get(), _1, _2, _3);

    std::function<void(
        TLog& Log,
        EBlockStoreRequest requestType,
        ui64 requestId,
        const TString& diskId,
        const TString& clientId)> ReportExceptionHandler
            = std::bind(&IServerStats::ReportException, Stub.get(), _1, _2, _3, _4, _5);

    std::function<void(
        const TIncompleteRequest& request)> AddIncompleteRequestHandler
            = std::bind(&IServerStats::AddIncompleteRequest, Stub.get(), _1);

    std::function<void(bool updatePercentiles)> UpdateStatsHandler
        = std::bind(&IServerStats::UpdateStats, Stub.get(), _1);

    TExecutorCounters::TExecutorScope StartExecutor() override
    {
        return Stub->StartExecutor();
    }

    NMonitoring::TDynamicCounters::TCounterPtr
        GetUnalignedRequestCounter() override
    {
        return Stub->GetUnalignedRequestCounter();
    }

    NMonitoring::TDynamicCounters::TCounterPtr
        GetEndpointCounter(NProto::EClientIpcType ipcType) override
    {
        return GetEndpointCounterHandler(ipcType);
    }

    bool MountVolume(
        const NProto::TVolume& volume,
        const TString& clientId,
        const TString& instanceId) override
    {
        return MountVolumeHandler(volume, clientId, instanceId);
    }

    void UnmountVolume(
        const TString& diskId,
        const TString& clientId) override
    {
        return UnmountVolumeHandler(diskId, clientId);
    }

    ui32 GetBlockSize(const TString& diskId) const override
    {
        return GetBlockSizeHandler(diskId);
    }

    void PrepareMetricRequest(
        TMetricRequest& metricRequest,
        TString clientId,
        TString diskId,
        ui64 startIndex,
        ui32 requestBytes) override
    {
        PrepareMetricRequestHandler(
            metricRequest,
            std::move(clientId),
            std::move(diskId),
            startIndex,
            requestBytes);
    }

    void RequestStarted(
        TLog& log,
        TMetricRequest& metricRequest,
        TCallContext& callContext,
        const TString& message) override
    {
        RequestStartedHandler(log, metricRequest, callContext, message);
    }

    void RequestAcquired(
        TMetricRequest& metricRequest,
        TCallContext& callContext) override
    {
        RequestAcquiredHandler(metricRequest, callContext);
    }

    void RequestSent(
        TMetricRequest& metricRequest,
        TCallContext& callContext) override
    {
        RequestSentHandler(metricRequest, callContext);
    }

    void ResponseReceived(
        TMetricRequest& metricRequest,
        TCallContext& callContext) override
    {
        ResponseReceivedHandler(metricRequest, callContext);
    }

    void ResponseSent(
        TMetricRequest& metricRequest,
        TCallContext& callContext) override
    {
        ResponseSentHandler(metricRequest, callContext);
    }

    void RequestCompleted(
        TLog& log,
        TMetricRequest& metricRequest,
        TCallContext& callContext,
        const NProto::TError& error) override
    {
        RequestCompletedHandler(log, metricRequest, callContext, error);
    }

    void RequestFastPathHit(
        const TString& diskId,
        const TString& clientId,
        EBlockStoreRequest requestType) override
    {
        RequestFastPathHitHandler(diskId, clientId, requestType);
    }

    void ReportException(
        TLog& Log,
        EBlockStoreRequest requestType,
        ui64 requestId,
        const TString& diskId,
        const TString& clientId) override
    {
        ReportExceptionHandler(
            Log,
            requestType,
            requestId,
            diskId,
            clientId);
    }

    void AddIncompleteRequest(const TIncompleteRequest& request) override
    {
        AddIncompleteRequestHandler(request);
    }

    void UpdateStats(bool updatePercentiles) override
    {
        UpdateStatsHandler(updatePercentiles);
    }
};

}   // namespace NCloud::NBlockStore
