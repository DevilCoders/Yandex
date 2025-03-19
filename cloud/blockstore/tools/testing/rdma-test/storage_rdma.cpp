#include "storage.h"

#include "probes.h"
#include "protocol.h"

#include <cloud/blockstore/tools/testing/rdma-test/protocol.pb.h>

#include <cloud/blockstore/libs/rdma/client.h>
#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/service/context.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/task_queue.h>

#include <util/datetime/base.h>

namespace NCloud::NBlockStore {

using namespace NThreading;

LWTRACE_USING(BLOCKSTORE_TEST_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr size_t MAX_PROTO_SIZE = 4*1024;

////////////////////////////////////////////////////////////////////////////////

struct IRequestHandler
{
    virtual ~IRequestHandler() = default;

    virtual void HandleResponse(TStringBuf buffer) = 0;
    virtual void HandleError(ui32 error, TString message) = 0;
};

using IRequestHandlerPtr = std::unique_ptr<IRequestHandler>;

////////////////////////////////////////////////////////////////////////////////

struct TReadBlocksHandler final
    : public IRequestHandler
{
    TCallContextPtr CallContext;
    TReadBlocksRequestPtr Request;
    TGuardedSgList GuardedSgList;
    TPromise<NProto::TError> Response;

    NRdma::TProtoMessageSerializer* Serializer = TBlockStoreProtocol::Serializer();

    TReadBlocksHandler(
            TCallContextPtr callContext,
            TReadBlocksRequestPtr request,
            TGuardedSgList guardedSgList,
            TPromise<NProto::TError> response)
        : CallContext(std::move(callContext))
        , Request(std::move(request))
        , GuardedSgList(std::move(guardedSgList))
        , Response(std::move(response))
    {}

    TResultOrError<NRdma::TClientRequestPtr> PrepareRequest(
        NRdma::IClientEndpoint& endpoint)
    {
        LWTRACK(
            RdmaPrepareRequest,
            CallContext->LWOrbit,
            CallContext->RequestId);

        size_t dataSize = Request->GetBlockSize() * Request->GetBlocksCount();

        auto [req, err] = endpoint.AllocateRequest(
            this,
            Serializer->MessageByteSize(*Request, 0),
            MAX_PROTO_SIZE + dataSize);

        if (HasError(err)) {
            return err;
        }

        Serializer->Serialize(
            req->RequestBuffer,
            TBlockStoreProtocol::ReadBlocksRequest,
            *Request,
            TContIOVector(nullptr, 0));

        return std::move(req);
    }

    void HandleResponse(TStringBuf buffer) override
    {
        LWTRACK(
            RdmaHandleResponse,
            CallContext->LWOrbit,
            CallContext->RequestId);

        auto resultOrError = Serializer->Parse(buffer);
        if (HasError(resultOrError)) {
            Response.SetValue(resultOrError.GetError());
            return;
        }

        const auto& response = resultOrError.GetResult();
        Y_ENSURE(response.MsgId == TBlockStoreProtocol::ReadBlocksResponse);

        auto guard = GuardedSgList.Acquire();
        Y_VERIFY(guard);

        size_t bytesCopied = CopyMemory(guard.Get(), response.Data);
        Y_ENSURE(bytesCopied == response.Data.length());

        Response.SetValue({});
    }

    void HandleError(ui32 error, TString message) override
    {
        Response.SetValue(MakeError(error, std::move(message)));
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TWriteBlocksHandler final
    : public IRequestHandler
{
    TCallContextPtr CallContext;
    TWriteBlocksRequestPtr Request;
    TGuardedSgList GuardedSgList;
    TPromise<NProto::TError> Response;

    NRdma::TProtoMessageSerializer* Serializer = TBlockStoreProtocol::Serializer();

    TWriteBlocksHandler(
            TCallContextPtr callContext,
            TWriteBlocksRequestPtr request,
            TGuardedSgList guardedSgList,
            TPromise<NProto::TError> response)
        : CallContext(std::move(callContext))
        , Request(std::move(request))
        , GuardedSgList(std::move(guardedSgList))
        , Response(std::move(response))
    {}

    TResultOrError<NRdma::TClientRequestPtr> PrepareRequest(
        NRdma::IClientEndpoint& endpoint)
    {
        size_t dataSize = Request->GetBlockSize() * Request->GetBlocksCount();

        auto [req, err] = endpoint.AllocateRequest(
            this,
            Serializer->MessageByteSize(*Request, dataSize),
            MAX_PROTO_SIZE);

        if (HasError(err)) {
            return err;
        }

        auto guard = GuardedSgList.Acquire();
        Y_VERIFY(guard);

        const auto& sglist = guard.Get();
        Serializer->Serialize(
            req->RequestBuffer,
            TBlockStoreProtocol::WriteBlocksRequest,
            *Request,
            TContIOVector((IOutputStream::TPart*)sglist.begin(), sglist.size()));

        return std::move(req);
    }

    void HandleResponse(TStringBuf buffer) override
    {
        auto resultOrError = Serializer->Parse(buffer);
        if (HasError(resultOrError)) {
            Response.SetValue(resultOrError.GetError());
            return;
        }

        const auto& response = resultOrError.GetResult();
        Y_ENSURE(response.MsgId == TBlockStoreProtocol::WriteBlocksResponse);

        Response.SetValue({});
    }

    void HandleError(ui32 error, TString message) override
    {
        Response.SetValue(MakeError(error, std::move(message)));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TRdmaStorage final
    : public IStorage
    , public NRdma::IClientHandler
{
private:
    ITaskQueuePtr TaskQueue;
    NRdma::IClientEndpointPtr Endpoint;

public:
    TRdmaStorage(ITaskQueuePtr taskQueue)
        : TaskQueue(std::move(taskQueue))
    {}

    void Init(NRdma::IClientEndpointPtr endpoint)
    {
        Endpoint = std::move(endpoint);
    }

    void Start() override {}
    void Stop() override {}

    TFuture<NProto::TError> ReadBlocks(
        TCallContextPtr callContext,
        TReadBlocksRequestPtr request,
        TGuardedSgList sglist) override
    {
        auto response = NewPromise<NProto::TError>();
        auto future = response.GetFuture();

        auto handler = std::make_unique<TReadBlocksHandler>(
            std::move(callContext),
            std::move(request),
            std::move(sglist),
            std::move(response));

        TaskQueue->ExecuteSimple([=, handler = std::move(handler)] () mutable {
            auto [req, err] = handler->PrepareRequest(*Endpoint);

            if (HasError(err)) {
                handler->Response.SetValue(err);
                return;
            }

            Endpoint->SendRequest(std::move(req), handler->CallContext);
            handler.release();  // ownership transferred
        });

        return future;
    }

    TFuture<NProto::TError> WriteBlocks(
        TCallContextPtr callContext,
        TWriteBlocksRequestPtr request,
        TGuardedSgList sglist) override
    {
        auto response = NewPromise<NProto::TError>();
        auto future = response.GetFuture();

        auto handler = std::make_unique<TWriteBlocksHandler>(
            std::move(callContext),
            std::move(request),
            std::move(sglist),
            std::move(response));

        TaskQueue->ExecuteSimple([=, handler = std::move(handler)] () mutable {
            auto [req, err] = handler->PrepareRequest(*Endpoint);

            if (HasError(err)) {
                handler->Response.SetValue(err);
                return;
            }

            Endpoint->SendRequest(std::move(req), handler->CallContext);
            handler.release();  // ownership transferred
        });

        return future;
    }

private:
    void HandleResponse(
        NRdma::TClientRequestPtr req,
        ui32 status,
        size_t responseBytes) override
    {
        IRequestHandlerPtr handler(static_cast<IRequestHandler*>(req->Context));

        TaskQueue->ExecuteSimple([
            =,
            req = std::move(req),
            handler = std::move(handler)
        ] () mutable {
            auto buffer = req->ResponseBuffer.Head(responseBytes);
            if (status == 0) {
                handler->HandleResponse(buffer);
            } else {
                handler->HandleError(status, TString(buffer));
            }

            Endpoint->FreeRequest(std::move(req));
        });
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IStoragePtr CreateRdmaStorage(
    NRdma::IClientPtr client,
    ITaskQueuePtr taskQueue,
    const TString& address,
    ui32 port)
{
    auto storage = std::make_shared<TRdmaStorage>(std::move(taskQueue));

    auto startEndpoint = client->StartEndpoint(address, port, storage);
    storage->Init(startEndpoint.GetValue(WAIT_TIMEOUT));

    return storage;
}

}   // namespace NCloud::NBlockStore
