#include "rdma_target.h"

#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/rdma/server.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service_local/rdma_protocol.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <cloud/storage/core/libs/common/task_queue.h>
#include <cloud/storage/core/libs/common/thread_pool.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/containers/stack_vector/stack_vec.h>

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration LOG_THROTTLER_PERIOD = TDuration::MilliSeconds(500);

}   // namespace

namespace NCloud::NBlockStore::NStorage {

using namespace NThreading;

////////////////////////////////////////////////////////////////////////////////

class TRequestHandler final
    : public NRdma::IServerHandler
{
private:
    THashMap<TString, TStorageAdapterPtr> Devices;
    ITaskQueuePtr TaskQueue;

    TLogThrottler& LogThrottler;
    TLog Log;

    NRdma::IServerEndpointPtr Endpoint;
    NRdma::TProtoMessageSerializer* Serializer
        = TBlockStoreProtocol::Serializer();

public:
    TRequestHandler(
            THashMap<TString, TStorageAdapterPtr> devices,
            ITaskQueuePtr taskQueue,
            TLogThrottler& logThrottler)
        : Devices(std::move(devices))
        , TaskQueue(std::move(taskQueue))
        , LogThrottler(logThrottler)
    {}

    void Init(NRdma::IServerEndpointPtr endpoint, TLog log)
    {
        Endpoint = std::move(endpoint);
        Log = std::move(log);
    }

    void HandleRequest(
        void* context,
        TCallContextPtr callContext,
        TStringBuf in,
        TStringBuf out) override
    {
        TaskQueue->ExecuteSimple([=] {
            auto error = SafeExecute<NProto::TError>([=] {
                return DoHandleRequest(context, callContext, in, out);
            });

            if (HasError(error)) {
                Endpoint->SendError(
                    context,
                    error.GetCode(),
                    error.GetMessage());
            }
        });
    }

private:
    NProto::TError DoHandleRequest(
        void* context,
        TCallContextPtr callContext,
        TStringBuf in,
        TStringBuf out)
    {
        auto resultOrError = Serializer->Parse(in);

        if (HasError(resultOrError)) {
            return resultOrError.GetError();
        }

        const auto& request = resultOrError.GetResult();

        switch (request.MsgId) {
            case TBlockStoreProtocol::ReadDeviceBlocksRequest:
                return HandleReadBlocksRequest(
                    context,
                    callContext,
                    static_cast<NProto::TReadDeviceBlocksRequest&>(*request.Proto),
                    request.Data,
                    out);

            case TBlockStoreProtocol::WriteDeviceBlocksRequest:
                return HandleWriteBlocksRequest(
                    context,
                    callContext,
                    static_cast<NProto::TWriteDeviceBlocksRequest&>(*request.Proto),
                    request.Data,
                    out);

            case TBlockStoreProtocol::ZeroDeviceBlocksRequest:
                return HandleZeroBlocksRequest(
                    context,
                    callContext,
                    static_cast<NProto::TZeroDeviceBlocksRequest&>(*request.Proto),
                    request.Data,
                    out);

            default:
                return MakeError(E_NOT_IMPLEMENTED);
        }
    }

    TStorageAdapterPtr GetDevice(const TString& uuid, const TString& sessionId)
    {
        // TODO
        Y_UNUSED(sessionId);

        auto it = Devices.find(uuid);

        if (it == Devices.cend()) {
            ythrow TServiceError(E_NOT_FOUND);
        }

        return it->second;
    }

    NProto::TError HandleReadBlocksRequest(
        void* context,
        TCallContextPtr callContext,
        NProto::TReadDeviceBlocksRequest& request,
        TStringBuf requestData,
        TStringBuf out)
    {
        if (Y_UNLIKELY(requestData.length() != 0)) {
            return MakeError(E_ARGUMENT);
        }

        auto device = GetDevice(
            request.GetDeviceUUID(),
            request.GetSessionId());

        auto req = std::make_shared<NProto::TReadBlocksRequest>();

        req->SetStartIndex(request.GetStartIndex());
        req->SetBlocksCount(request.GetBlocksCount());

        auto future = device->ReadBlocks(
            std::move(callContext),
            std::move(req),
            request.GetBlockSize());

        future.Subscribe([=, &request] (auto future) {
            TaskQueue->ExecuteSimple([=, &request] {
                HandleReadBlocksResponse(context, out, request, future);
            });
        });

        return {};
    }

    void HandleReadBlocksResponse(
        void* context,
        TStringBuf out,
        NProto::TReadDeviceBlocksRequest& request,
        TFuture<NProto::TReadBlocksResponse> future)
    {
        auto& response = future.GetValue();
        auto& blocks = response.GetBlocks();
        auto& error = response.GetError();

        NProto::TReadDeviceBlocksResponse proto;

        if (HasError(error)) {
            STORAGE_ERROR_T(LogThrottler, "[" << request.GetDeviceUUID()
                << "/" << request.GetSessionId() << "] read error: "
                << error.GetMessage() << " (" << error.GetCode() << ")");

            *proto.MutableError() = error;
        }

        TStackVec<IOutputStream::TPart> parts(blocks.BuffersSize());

        for (auto buffer: blocks.GetBuffers()) {
            parts.emplace_back(TStringBuf(buffer));
        }

        size_t bytes = Serializer->Serialize(
            out,
            TBlockStoreProtocol::ReadDeviceBlocksResponse,
            proto,
            TContIOVector(parts.data(), parts.size()));

        Endpoint->SendResponse(context, bytes);
    }

    NProto::TError HandleWriteBlocksRequest(
        void* context,
        TCallContextPtr callContext,
        NProto::TWriteDeviceBlocksRequest& request,
        TStringBuf requestData,
        TStringBuf out)
    {
        if (Y_UNLIKELY(requestData.length() == 0)) {
            return MakeError(E_ARGUMENT);
        }

        auto device = GetDevice(
            request.GetDeviceUUID(),
            request.GetSessionId());

        auto req = std::make_shared<NProto::TWriteBlocksRequest>();

        req->SetStartIndex(request.GetStartIndex());
        req->MutableBlocks()->AddBuffers(
            requestData.data(),
            requestData.length());

        auto future = device->WriteBlocks(
            std::move(callContext),
            std::move(req),
            request.GetBlockSize());

        future.Subscribe([=, &request] (auto future) {
            TaskQueue->ExecuteSimple([=, &request] {
                HandleWriteBlocksResponse(context, out, request, future);
            });
        });

        return {};
    }

    void HandleWriteBlocksResponse(
        void* context,
        TStringBuf out,
        NProto::TWriteDeviceBlocksRequest &request,
        TFuture<NProto::TWriteBlocksResponse> future)
    {
        auto& response = future.GetValue();
        auto& error = response.GetError();

        NProto::TWriteDeviceBlocksResponse proto;

        if (HasError(error)) {
            STORAGE_ERROR_T(LogThrottler, "[" << request.GetDeviceUUID()
                << "/" << request.GetSessionId() << "] write error: "
                << error.GetMessage() << " (" << error.GetCode() << ")");

            *proto.MutableError() = error;
        }

        size_t bytes = Serializer->Serialize(
            out,
            TBlockStoreProtocol::WriteDeviceBlocksResponse,
            proto,
            TContIOVector(nullptr, 0));

        Endpoint->SendResponse(context, bytes);
    }

    NProto::TError HandleZeroBlocksRequest(
        void* context,
        TCallContextPtr callContext,
        NProto::TZeroDeviceBlocksRequest &request,
        TStringBuf requestData,
        TStringBuf out)
    {
        if (Y_UNLIKELY(requestData.length() != 0)) {
            return MakeError(E_ARGUMENT);
        }

        auto device = GetDevice(
            request.GetDeviceUUID(),
            request.GetSessionId());

        auto req = std::make_shared<NProto::TZeroBlocksRequest>();

        req->SetStartIndex(request.GetStartIndex());
        req->SetBlocksCount(request.GetBlocksCount());

        auto future = device->ZeroBlocks(
            std::move(callContext),
            std::move(req),
            request.GetBlockSize());

        future.Subscribe([=, &request] (auto future) {
            TaskQueue->ExecuteSimple([=, &request] {
                HandleZeroBlocksResponse(context, out, request, future);
            });
        });

        return {};
    }

    void HandleZeroBlocksResponse(
        void* context,
        TStringBuf out,
        NProto::TZeroDeviceBlocksRequest &request,
        TFuture<NProto::TZeroBlocksResponse> future)
    {
        auto& response = future.GetValue();
        auto& error = response.GetError();

        NProto::TZeroDeviceBlocksResponse proto;

        if (HasError(error)) {
            STORAGE_ERROR_T(LogThrottler, "[" << request.GetDeviceUUID()
                << "/" << request.GetSessionId() << "] zero error: "
                << error.GetMessage() << " (" << error.GetCode() << ")");

            *proto.MutableError() = error;
        }

        size_t bytes = Serializer->Serialize(
            out,
            TBlockStoreProtocol::ZeroDeviceBlocksResponse,
            proto,
            TContIOVector(nullptr, 0));
        
        Endpoint->SendResponse(context, bytes);
    }
};

using TRequestHandlerPtr = std::shared_ptr<TRequestHandler>;

class TRdmaTarget final
    : public IRdmaTarget
{
private:
    const NProto::TRdmaEndpoint Config;

    TRequestHandlerPtr Handler;
    ILoggingServicePtr Logging;
    TLogThrottler LogThrottler;
    NRdma::IServerPtr Server;

public:
    TRdmaTarget(
            NProto::TRdmaEndpoint config,
            ILoggingServicePtr logging,
            TLogThrottler logThrottler,
            NRdma::IServerPtr server,
            THashMap<TString, TStorageAdapterPtr> devices,
            ITaskQueuePtr taskQueue)
        : Config(std::move(config))
        , Logging(std::move(logging))
        , LogThrottler(std::move(logThrottler))
        , Server(std::move(server))
    {
        Handler = std::make_shared<TRequestHandler>(
            std::move(devices),
            std::move(taskQueue),
            LogThrottler);
    }

    void Start() {
        auto endpoint = Server->StartEndpoint(
            Config.GetHost(),
            Config.GetPort(),
            Handler);

        Handler->Init(endpoint, Logging->CreateLog("BLOCKSTORE_DISK_AGENT"));
    }

    void Stop()
    {}
};

IRdmaTargetPtr CreateRdmaTarget(
    NProto::TRdmaEndpoint config,
    ILoggingServicePtr logging,
    NRdma::IServerPtr server,
    THashMap<TString, TStorageAdapterPtr> devices)
{
    auto logThrottler = TLogThrottler(LOG_THROTTLER_PERIOD);

    // TODO
    auto threadPool = CreateThreadPool("RDMA", 1);
    threadPool->Start();

    return std::make_shared<TRdmaTarget>(
        std::move(config),
        std::move(logging),
        std::move(logThrottler),
        std::move(server),
        std::move(devices),
        std::move(threadPool));
}

}   // namespace NCloud::NBlockStore::NStorage
