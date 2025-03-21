#include "socket_endpoint_listener.h"

#include <cloud/blockstore/libs/client/session.h>
#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/endpoints/endpoint_listener.h>
#include <cloud/blockstore/libs/server/endpoint_poller.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/blockstore/libs/service/storage.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;
using namespace NCloud::NBlockStore::NClient;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TEndpointServiceBase
    : public IBlockStore
{
public:
#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) override            \
        {                                                                      \
            return CreateUnsupportedResponse<NProto::T##name##Response>(       \
                std::move(callContext),                                        \
                std::move(request));                                           \
        }                                                                      \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_IMPLEMENT_METHOD)

#undef BLOCKSTORE_IMPLEMENT_METHOD

private:
    template <typename TResponse, typename TRequest>
    TFuture<TResponse> CreateUnsupportedResponse(
        TCallContextPtr callContext,
        std::shared_ptr<TRequest> request)
    {
        Y_UNUSED(callContext);
        Y_UNUSED(request);

        auto requestType = GetBlockStoreRequest<TRequest>();
        const auto& requestName = GetBlockStoreRequestName(requestType);

        return MakeFuture<TResponse>(TErrorResponse(
            E_FAIL,
            TStringBuilder()
                << "Unsupported endpoint request: " << requestName.Quote()));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TEndpointService final
    : public TEndpointServiceBase
{
private:
    const TString DiskId;
    const ui32 BlockSize;
    const TStorageAdapter StorageAdapter;
    const ISessionPtr Session;

public:
    TEndpointService(
            TString diskId,
            ui32 blockSize,
            ISessionPtr session)
        : DiskId(std::move(diskId))
        , BlockSize(blockSize)
        , StorageAdapter(session, blockSize, true)
        , Session(std::move(session))
    {}

    void Start() override
    {}

    void Stop() override
    {}

    TStorageBuffer AllocateBuffer(size_t bytesCount) override
    {
        Y_UNUSED(bytesCount);
        return nullptr;
    }

    TFuture<NProto::TMountVolumeResponse> MountVolume(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TMountVolumeRequest> request) override
    {
        Y_UNUSED(callContext);

        auto response = ValidateRequest<NProto::TMountVolumeResponse>(*request);
        if (HasError(response)) {
            return MakeFuture(response);
        }

        return Session->EnsureVolumeMounted();
    }

    TFuture<NProto::TUnmountVolumeResponse> UnmountVolume(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TUnmountVolumeRequest> request) override
    {
        Y_UNUSED(callContext);

        auto response = ValidateRequest<NProto::TUnmountVolumeResponse>(*request);
        if (HasError(response)) {
            return MakeFuture(response);
        }

        auto& error = *response.MutableError();
        error.SetCode(S_FALSE);
        error.SetMessage("Emulated unmount response");
        return MakeFuture(response);
    }

    TFuture<NProto::TReadBlocksResponse> ReadBlocks(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TReadBlocksRequest> request) override
    {
        auto res = ValidateRequest<NProto::TReadBlocksResponse>(*request);
        if (HasError(res)) {
            return MakeFuture(res);
        }

        return StorageAdapter.ReadBlocks(
            std::move(callContext),
            std::move(request),
            BlockSize);
    }

    TFuture<NProto::TWriteBlocksResponse> WriteBlocks(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TWriteBlocksRequest> request) override
    {
        auto response = ValidateRequest<NProto::TWriteBlocksResponse>(*request);
        if (HasError(response)) {
            return MakeFuture(response);
        }

        return StorageAdapter.WriteBlocks(
            std::move(callContext),
            std::move(request),
            BlockSize);
    }

    TFuture<NProto::TZeroBlocksResponse> ZeroBlocks(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TZeroBlocksRequest> request) override
    {
        auto response = ValidateRequest<NProto::TZeroBlocksResponse>(*request);
        if (HasError(response)) {
            return MakeFuture(response);
        }

        return StorageAdapter.ZeroBlocks(
            std::move(callContext),
            std::move(request),
            BlockSize);
    }

private:
    template <typename TResponse, typename TRequest>
    TResponse ValidateRequest(TRequest& request)
    {
        const auto& diskId = request.GetDiskId();
        if (diskId != DiskId) {
            return TErrorResponse(
                E_ARGUMENT,
                TStringBuilder() << "invalid disk id: " << diskId);
        }

        return TResponse();
    }
};

////////////////////////////////////////////////////////////////////////////////

class TSocketEndpointListener final
    : public ISocketEndpointListener
{
private:
    const ui32 UnixSocketBacklog;

    std::unique_ptr<TEndpointPoller> EndpointPoller;

    TLog Log;

public:
    TSocketEndpointListener(
            ILoggingServicePtr logging,
            ui32 unixSocketBacklog)
        : UnixSocketBacklog(unixSocketBacklog)
    {
        Log = logging->CreateLog("BLOCKSTORE_SERVER");
    }

    ~TSocketEndpointListener()
    {
        Stop();
    }

    void SetClientAcceptor(IClientAcceptorPtr clientAcceptor) override
    {
        Y_VERIFY(!EndpointPoller);
        EndpointPoller = std::make_unique<TEndpointPoller>(
            std::move(clientAcceptor));
    }

    void Start() override
    {
        Y_VERIFY(EndpointPoller);
        EndpointPoller->Start();
    }

    void Stop() override
    {
        if (EndpointPoller) {
            EndpointPoller->Stop();
        }
    }

    TFuture<NProto::TError> StartEndpoint(
        const NProto::TStartEndpointRequest& request,
        const NProto::TVolume& volume,
        ISessionPtr session) override
    {
        auto sessionService = std::make_shared<TEndpointService>(
            request.GetDiskId(),
            volume.GetBlockSize(),
            std::move(session));

        auto error = EndpointPoller->StartListenEndpoint(
            request.GetUnixSocketPath(),
            UnixSocketBacklog,
            false,  // multiClient
            std::move(sessionService));

        return MakeFuture(std::move(error));
    }

    TFuture<NProto::TError> StopEndpoint(
        const TString& socketPath) override
    {
        auto error = EndpointPoller->StopListenEndpoint(socketPath);
        return MakeFuture(std::move(error));
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

ISocketEndpointListenerPtr CreateSocketEndpointListener(
    ILoggingServicePtr logging,
    ui32 unixSocketBacklog)
{
    return std::make_unique<TSocketEndpointListener>(
        std::move(logging),
        unixSocketBacklog);
}

}   // namespace NCloud::NBlockStore::NServer
