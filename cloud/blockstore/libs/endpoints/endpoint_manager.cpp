#include "endpoint_manager.h"

#include "endpoint_listener.h"
#include "session_manager.h"

#include <cloud/blockstore/libs/client/config.h>
#include <cloud/blockstore/libs/client/session.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/coroutine/executor.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <util/generic/hash.h>
#include <util/string/builder.h>
#include <util/system/mutex.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;

using namespace NCloud::NBlockStore::NClient;

namespace {

////////////////////////////////////////////////////////////////////////////////

bool CompareRequests(
    const NProto::TKmsSpec& left,
    const NProto::TKmsSpec& right)
{
    return left.GetKekId() == right.GetKekId()
        && left.GetEncryptedDEK() == right.GetEncryptedDEK();
}

bool CompareRequests(
    const NProto::TKeyPath& left,
    const NProto::TKeyPath& right)
{
    return CompareRequests(left.GetKmsSpec(), right.GetKmsSpec())
        && left.GetFilePath() == right.GetFilePath()
        && left.GetKeyringId() == right.GetKeyringId()
        && left.GetDEK() == right.GetDEK();
}

bool CompareRequests(
    const NProto::TEncryptionKey& left,
    const NProto::TEncryptionKey& right)
{
    return left.GetMode() == right.GetMode()
        && CompareRequests(left.GetKeyPath(), right.GetKeyPath());
}

bool CompareRequests(
    const NProto::TEncryptionSpec& left,
    const NProto::TEncryptionSpec& right)
{
    return CompareRequests(left.GetKey(), right.GetKey())
        && left.GetKeyHash() == right.GetKeyHash()
        && left.GetDisableEncryption() == right.GetDisableEncryption();
}

bool CompareRequests(
    const NProto::TClientProfile& left,
    const NProto::TClientProfile& right)
{
    return left.GetCpuUnitCount() == right.GetCpuUnitCount();
}

bool CompareRequests(
    const NProto::TClientMediaKindPerformanceProfile& left,
    const NProto::TClientMediaKindPerformanceProfile& right)
{
    return left.GetMaxReadIops() == right.GetMaxReadIops()
        && left.GetMaxWriteIops() == right.GetMaxWriteIops()
        && left.GetMaxReadBandwidth() == right.GetMaxReadBandwidth()
        && left.GetMaxWriteBandwidth() == right.GetMaxWriteBandwidth();
}

bool CompareRequests(
    const NProto::TClientPerformanceProfile& left,
    const NProto::TClientPerformanceProfile& right)
{
    return CompareRequests(left.GetHDDProfile(), right.GetHDDProfile())
        && CompareRequests(left.GetSSDProfile(), right.GetSSDProfile())
        && CompareRequests(left.GetNonreplProfile(), right.GetNonreplProfile())
        && left.GetBurstTime() == right.GetBurstTime();
}

bool CompareRequests(
    const NProto::TStartEndpointRequest& left,
    const NProto::TStartEndpointRequest& right)
{
    // TODO(NBS-2032): compare instance ids
    return left.GetUnixSocketPath() == right.GetUnixSocketPath()
        && left.GetDiskId() == right.GetDiskId()
        && left.GetVolumeAccessMode() == right.GetVolumeAccessMode()
        && left.GetVolumeMountMode() == right.GetVolumeMountMode()
        && left.GetIpcType() == right.GetIpcType()
        && left.GetClientVersionInfo() == right.GetClientVersionInfo()
        && left.GetMountFlags() == right.GetMountFlags()
        && left.GetMountSeqNumber() == right.GetMountSeqNumber()
        && CompareRequests(left.GetEncryptionSpec(), right.GetEncryptionSpec())
        && left.GetClientId() == right.GetClientId()
        && CompareRequests(left.GetClientProfile(), right.GetClientProfile())
        && CompareRequests(left.GetClientPerformanceProfile(), right.GetClientPerformanceProfile())
        && left.GetVhostQueuesCount() == right.GetVhostQueuesCount()
        && left.GetUnalignedRequestsDisabled() == right.GetUnalignedRequestsDisabled();
}

////////////////////////////////////////////////////////////////////////////////

struct TStartingEndpointState
{
    NProto::TStartEndpointRequest Request;
    TFuture<NProto::TStartEndpointResponse> Result;
};

////////////////////////////////////////////////////////////////////////////////

class TEndpointManager final
    : public IEndpointManager
    , public std::enable_shared_from_this<TEndpointManager>
{
private:
    const IServerStatsPtr ServerStats;
    const TExecutorPtr Executor;
    const ISessionManagerPtr SessionManager;
    const THashMap<NProto::EClientIpcType, IEndpointListenerPtr> EndpointListeners;
    const TString NbdSocketSuffix;

    TLog Log;

    TMutex ProcessingLock;
    THashMap<TString, TStartingEndpointState> StartingSockets;
    THashMap<TString, TFuture<NProto::TStopEndpointResponse>> StoppingSockets;

    THashMap<TString, std::shared_ptr<NProto::TStartEndpointRequest>> Requests;

public:
    TEndpointManager(
            ILoggingServicePtr logging,
            IServerStatsPtr serverStats,
            TExecutorPtr executor,
            ISessionManagerPtr sessionManager,
            THashMap<NProto::EClientIpcType, IEndpointListenerPtr> listeners,
            TString nbdSocketSuffix)
        : ServerStats(std::move(serverStats))
        , Executor(std::move(executor))
        , SessionManager(std::move(sessionManager))
        , EndpointListeners(std::move(listeners))
        , NbdSocketSuffix(std::move(nbdSocketSuffix))
    {
        Log = logging->CreateLog("BLOCKSTORE_SERVER");
    }

    TFuture<NProto::TStartEndpointResponse> StartEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TStartEndpointRequest> request) override;

    TFuture<NProto::TStopEndpointResponse> StopEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TStopEndpointRequest> request) override;

    TFuture<NProto::TListEndpointsResponse> ListEndpoints(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TListEndpointsRequest> request) override;

    TFuture<NProto::TDescribeEndpointResponse> DescribeEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TDescribeEndpointRequest> request) override;

private:
    NProto::TStartEndpointResponse StartEndpointImpl(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TStartEndpointRequest> request);

    NProto::TStopEndpointResponse StopEndpointImpl(
        TCallContextPtr ctx,
        const TString& socketPath,
        const NProto::THeaders& headers);

    NProto::TListEndpointsResponse ListEndpointsImpl(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TListEndpointsRequest> request);

    NProto::TStartEndpointResponse AlterEndpoint(
        TCallContextPtr ctx,
        const NProto::TStartEndpointRequest& newRequest,
        const NProto::TStartEndpointRequest& oldRequest);

    NProto::TError OpenEndpointSocket(
        const NProto::TStartEndpointRequest& request,
        const TSessionInfo& sessionInfo);

    TFuture<NProto::TError> CloseEndpointSocket(
        const NProto::TStartEndpointRequest& startRequest);

    std::shared_ptr<NProto::TStartEndpointRequest> CreateNbdStartEndpointRequest(
        const NProto::TStartEndpointRequest& request);

    template <typename T>
    void RemoveProcessingSocket(
        THashMap<TString, T>& socketMap,
        const TString& socket);
};

////////////////////////////////////////////////////////////////////////////////

TFuture<NProto::TStartEndpointResponse> TEndpointManager::StartEndpoint(
    TCallContextPtr ctx,
    std::shared_ptr<NProto::TStartEndpointRequest> request)
{
    TFuture<NProto::TStartEndpointResponse> result;

    auto socketPath = request->GetUnixSocketPath();

    with_lock (ProcessingLock) {
        if (StoppingSockets.find(socketPath) != StoppingSockets.end()) {
            auto response = TErrorResponse(
                E_REJECTED,
                TStringBuilder()
                    << "endpoint " << socketPath.Quote()
                    << " is stopping now");
            return MakeFuture<NProto::TStartEndpointResponse>(std::move(response));
        }

        auto it = StartingSockets.find(socketPath);
        if (it != StartingSockets.end()) {
            const auto& state = it->second;
            if (CompareRequests(*request, state.Request)) {
                return state.Result;
            }

            auto response = TErrorResponse(
                E_REJECTED,
                TStringBuilder()
                    << "endpoint " << socketPath.Quote()
                    << " is starting now with other args");
            return MakeFuture<NProto::TStartEndpointResponse>(std::move(response));
        }

        TStartingEndpointState state;
        state.Request = *request;
        state.Result = Executor->Execute([=] () mutable {
            return StartEndpointImpl(std::move(ctx), std::move(request));
        });

        result = state.Result;
        StartingSockets.emplace(socketPath, std::move(state));
    }

    auto weak_ptr = weak_from_this();
    return result.Apply([weak_ptr, socketPath] (const auto& f) {
        if (auto p = weak_ptr.lock()) {
            p->RemoveProcessingSocket(p->StartingSockets, socketPath);
        }
        return f.GetValue();
    });
}

NProto::TStartEndpointResponse TEndpointManager::StartEndpointImpl(
    TCallContextPtr ctx,
    std::shared_ptr<NProto::TStartEndpointRequest> request)
{
    auto socketPath = request->GetUnixSocketPath();

    auto requestIt = Requests.find(socketPath);
    if (requestIt != Requests.end()) {
        const auto& startedEndpoint = *requestIt->second;
        return AlterEndpoint(std::move(ctx), *request, startedEndpoint);
    }

    if (request->GetClientId().empty()) {
        return TErrorResponse(
            E_ARGUMENT,
            TStringBuilder() << "ClientId shouldn't be empty");
    }

    auto createSessionFuture = SessionManager->CreateSession(ctx, *request);
    auto result = Executor->WaitFor(createSessionFuture);
    if (HasError(result)) {
        return TErrorResponse(result.GetError());
    }
    const auto& sessionInfo = result.GetResult();

    auto error = OpenEndpointSocket(*request, sessionInfo);
    if (HasError(error)) {
        auto future = SessionManager->RemoveSession(
            std::move(ctx),
            socketPath,
            request->GetHeaders());
        Executor->WaitFor(future);
        return TErrorResponse(error);
    }

    auto nbdRequest = CreateNbdStartEndpointRequest(*request);
    if (nbdRequest) {
        STORAGE_INFO("Start additional endpoint: " << *nbdRequest);
        auto error = OpenEndpointSocket(*nbdRequest, sessionInfo);

        if (HasError(error)) {
            auto closeFuture = CloseEndpointSocket(*request);
            Executor->WaitFor(closeFuture);
            auto removeFuture = SessionManager->RemoveSession(
                std::move(ctx),
                socketPath,
                request->GetHeaders());
            Executor->WaitFor(removeFuture);
            return TErrorResponse(error);
        }
    }

    if (auto c = ServerStats->GetEndpointCounter(request->GetIpcType())) {
        c->Inc();
    }
    auto [it, inserted] = Requests.emplace(socketPath, std::move(request));
    Y_VERIFY(inserted);

    NProto::TStartEndpointResponse response;
    response.MutableVolume()->CopyFrom(sessionInfo.Volume);
    return response;
}

NProto::TStartEndpointResponse TEndpointManager::AlterEndpoint(
    TCallContextPtr ctx,
    const NProto::TStartEndpointRequest& newRequest,
    const NProto::TStartEndpointRequest& oldRequest)
{
    const auto& socketPath = newRequest.GetUnixSocketPath();

    auto startedEndpoint = oldRequest;

    // NBS-3018
    if (!CompareRequests(
        oldRequest.GetClientProfile(),
        newRequest.GetClientProfile()))
    {
        STORAGE_WARN(TStringBuilder()
            << "ClientProfile has been changed for started endpoint: "
            << socketPath.Quote());

        startedEndpoint.MutableClientProfile()->CopyFrom(
            newRequest.GetClientProfile());
    }

    if (CompareRequests(newRequest, startedEndpoint)) {
        return TErrorResponse(
            S_ALREADY,
            TStringBuilder()
                << "endpoint " << socketPath.Quote()
                << " has already been started");
    }

    startedEndpoint.SetVolumeAccessMode(newRequest.GetVolumeAccessMode());
    startedEndpoint.SetVolumeMountMode(newRequest.GetVolumeMountMode());
    startedEndpoint.SetMountSeqNumber(newRequest.GetMountSeqNumber());

    if (!CompareRequests(newRequest, startedEndpoint)) {
        return TErrorResponse(
            E_INVALID_STATE,
            TStringBuilder()
                << "endpoint " << socketPath.Quote()
                << " has already been started with other args");
    }

    auto future = SessionManager->AlterSession(
        std::move(ctx),
        socketPath,
        newRequest.GetVolumeAccessMode(),
        newRequest.GetVolumeMountMode(),
        newRequest.GetMountSeqNumber(),
        newRequest.GetHeaders());
    auto error = Executor->WaitFor(future);

    return TErrorResponse(error);
}

TFuture<NProto::TStopEndpointResponse> TEndpointManager::StopEndpoint(
    TCallContextPtr ctx,
    std::shared_ptr<NProto::TStopEndpointRequest> request)
{
    TFuture<NProto::TStopEndpointResponse> result;

    auto socketPath = request->GetUnixSocketPath();

    with_lock (ProcessingLock) {
        if (StartingSockets.find(socketPath) != StartingSockets.end()) {
            auto response = TErrorResponse(
                E_REJECTED,
                TStringBuilder()
                    << "endpoint " << socketPath.Quote()
                    << " is starting now");
            return MakeFuture<NProto::TStopEndpointResponse>(std::move(response));
        }

        auto it = StoppingSockets.find(socketPath);
        if (it != StoppingSockets.end()) {
            return it->second;
        }

        result = Executor->Execute([=] () mutable {
            return StopEndpointImpl(std::move(ctx), socketPath, request->GetHeaders());
        });

        StoppingSockets.emplace(socketPath, result);
    }

    auto weak_ptr = weak_from_this();
    return result.Apply([weak_ptr, socketPath] (const auto& f) {
        if (auto p = weak_ptr.lock()) {
            p->RemoveProcessingSocket(p->StoppingSockets, socketPath);
        }
        return f.GetValue();
    });
}

NProto::TStopEndpointResponse TEndpointManager::StopEndpointImpl(
    TCallContextPtr ctx,
    const TString& socketPath,
    const NProto::THeaders& headers)
{
    auto it = Requests.find(socketPath);
    if (it == Requests.end()) {
        return TErrorResponse(
            S_FALSE,
            TStringBuilder()
                << "endpoint " << socketPath.Quote()
                << " hasn't been started yet");
    }

    auto startRequest = std::move(it->second);
    Requests.erase(it);
    if (auto c = ServerStats->GetEndpointCounter(startRequest->GetIpcType())) {
        c->Dec();
    }

    TVector<TFuture<NProto::TError>> closeSocketFutures;

    auto future = CloseEndpointSocket(*startRequest);
    closeSocketFutures.push_back(future);

    auto nbdRequest = CreateNbdStartEndpointRequest(*startRequest);
    if (nbdRequest) {
        STORAGE_INFO("Stop additional endpoint: "
            << nbdRequest->GetUnixSocketPath().Quote());
        auto future = CloseEndpointSocket(*nbdRequest);
        closeSocketFutures.push_back(future);
    }

    auto removeFuture = SessionManager->RemoveSession(
        std::move(ctx),
        socketPath,
        headers);
    Executor->WaitFor(removeFuture);

    NProto::TStopEndpointResponse response;

    for (const auto& closeSocketFuture: closeSocketFutures) {
        auto error = Executor->WaitFor(closeSocketFuture);

        if (HasError(error)) {
            response = TErrorResponse(error);
        }
    }

    return response;
}

TFuture<NProto::TListEndpointsResponse> TEndpointManager::ListEndpoints(
    TCallContextPtr ctx,
    std::shared_ptr<NProto::TListEndpointsRequest> request)
{
    return Executor->Execute([=] () mutable {
        return ListEndpointsImpl(std::move(ctx), std::move(request));
    });
}

NProto::TListEndpointsResponse TEndpointManager::ListEndpointsImpl(
    TCallContextPtr ctx,
    std::shared_ptr<NProto::TListEndpointsRequest> request)
{
    Y_UNUSED(ctx);
    Y_UNUSED(request);

    NProto::TListEndpointsResponse response;
    auto& endpoints = *response.MutableEndpoints();
    endpoints.Reserve(Requests.size());

    for (auto it: Requests) {
        auto& endpoint = *endpoints.Add();
        endpoint.CopyFrom(*it.second);
    }

    return response;
}

TFuture<NProto::TDescribeEndpointResponse> TEndpointManager::DescribeEndpoint(
    TCallContextPtr ctx,
    std::shared_ptr<NProto::TDescribeEndpointRequest> request)
{
    Y_UNUSED(ctx);

    return SessionManager->DescribeSession(request->GetUnixSocketPath());
}

NProto::TError TEndpointManager::OpenEndpointSocket(
    const NProto::TStartEndpointRequest& request,
    const TSessionInfo& sessionInfo)
{
    auto ipcType = request.GetIpcType();
    auto listenerIt = EndpointListeners.find(ipcType);
    if (listenerIt == EndpointListeners.end()) {
        return TErrorResponse(
            E_ARGUMENT,
            TStringBuilder()
                << "unsupported endpoint type: " << static_cast<ui32>(ipcType));
    }
    auto listener = listenerIt->second;

    if (request.GetUnixSocketPath().size() > UnixSocketPathLengthLimit) {
        return TErrorResponse(
            E_ARGUMENT,
            TStringBuilder()
                << "Length of socket path should not be more than "
                << UnixSocketPathLengthLimit);
    }

    auto future = listener->StartEndpoint(
        request,
        sessionInfo.Volume,
        sessionInfo.Session);

    auto error = Executor->WaitFor(future);
    return error;
}

TFuture<NProto::TError> TEndpointManager::CloseEndpointSocket(
    const NProto::TStartEndpointRequest& startRequest)
{
    auto ipcType = startRequest.GetIpcType();
    auto listenerIt = EndpointListeners.find(ipcType);
    Y_VERIFY(listenerIt != EndpointListeners.end());
    const auto& listener = listenerIt->second;

    return listener->StopEndpoint(startRequest.GetUnixSocketPath());
}

using TStartEndpointRequestPtr = std::shared_ptr<NProto::TStartEndpointRequest>;

TStartEndpointRequestPtr TEndpointManager::CreateNbdStartEndpointRequest(
    const NProto::TStartEndpointRequest& request)
{
    if (request.GetIpcType() != NProto::IPC_GRPC || NbdSocketSuffix.empty()) {
        return nullptr;
    }

    auto socketPath = request.GetUnixSocketPath() + NbdSocketSuffix;

    auto nbdRequest = std::make_shared<NProto::TStartEndpointRequest>(request);
    nbdRequest->SetIpcType(NProto::IPC_NBD);
    nbdRequest->SetUnixSocketPath(socketPath);
    nbdRequest->SetUnalignedRequestsDisabled(true);
    nbdRequest->SetSendNbdMinBlockSize(true);
    return nbdRequest;
}

template <typename T>
void TEndpointManager::RemoveProcessingSocket(
    THashMap<TString, T>& socketMap,
    const TString& socket)
{
    with_lock (ProcessingLock) {
        auto it = socketMap.find(socket);
        Y_VERIFY(it != socketMap.end());
        socketMap.erase(it);
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IEndpointManagerPtr CreateEndpointManager(
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    TExecutorPtr executor,
    ISessionManagerPtr sessionManager,
    THashMap<NProto::EClientIpcType, IEndpointListenerPtr> listeners,
    TString nbdSocketSuffix)
{
    return std::make_shared<TEndpointManager>(
        std::move(logging),
        std::move(serverStats),
        std::move(executor),
        std::move(sessionManager),
        std::move(listeners),
        std::move(nbdSocketSuffix));
}

}   // namespace NCloud::NBlockStore::NServer
