#include "client_test.h"

#include <cloud/blockstore/libs/common/sglist.h>
#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/service_local/rdma_protocol.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <util/generic/deque.h>
#include <util/string/printf.h>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TRdmaEndpointImpl
    : NRdma::IClientEndpoint
{
    NRdma::IClientHandlerPtr Handler;
    TDeque<TString> Blocks;
    NProto::TError AllocationError;
    NProto::TError ResponseError;

    TRdmaEndpointImpl(NRdma::IClientHandlerPtr handler)
        : Handler(std::move(handler))
    {}

    TResultOrError<NRdma::TClientRequestPtr> AllocateRequest(
        void* context,
        size_t requestBytes,
        size_t responseBytes) override
    {
        if (HasError(AllocationError)) {
            return AllocationError;
        }

        auto req = std::make_unique<NRdma::TClientRequest>();
        req->Context = context;
        req->RequestBuffer = {new char[requestBytes], requestBytes};
        req->ResponseBuffer = {new char[responseBytes], responseBytes};

        return std::move(req);
    }

    void SendRequest(
        NRdma::TClientRequestPtr req,
        TCallContextPtr callContext) override
    {
        Y_UNUSED(callContext);

        auto* serializer = TBlockStoreProtocol::Serializer();
        auto [result, err] = serializer->Parse(req->RequestBuffer);
        Y_ENSURE_EX(!HasError(err), yexception() << err.GetMessage());

        if (HasError(ResponseError)) {
            Handler->HandleResponse(std::move(req), ResponseError.GetCode(), 0);
            return;
        }

        size_t responseBytes = 0;
        switch (result.MsgId) {
            case TBlockStoreProtocol::ReadDeviceBlocksRequest: {
                NProto::TReadDeviceBlocksResponse response;
                TSgList sglist;
                using TProto = NProto::TReadDeviceBlocksRequest;
                auto* request = static_cast<TProto*>(result.Proto.get());
                const size_t minSize =
                    request->GetStartIndex() + request->GetBlocksCount();
                if (Blocks.size() < minSize) {
                    Blocks.resize(minSize, TString(4_KB, 0));
                }
                for (ui32 i = request->GetStartIndex(); i < minSize; ++i) {
                    sglist.emplace_back(Blocks[i].Data(), Blocks[i].Size());
                }

                responseBytes = serializer->Serialize(
                    req->ResponseBuffer,
                    TBlockStoreProtocol::ReadDeviceBlocksResponse,
                    response,
                    TContIOVector(
                        (IOutputStream::TPart*)sglist.begin(),
                        sglist.size()
                    ));

                break;
            }

            case TBlockStoreProtocol::WriteDeviceBlocksRequest: {
                NProto::TWriteDeviceBlocksResponse response;
                using TProto = NProto::TWriteDeviceBlocksRequest;
                auto* request = static_cast<TProto*>(result.Proto.get());
                const auto blockCount =
                    result.Data.Size() / request->GetBlockSize();
                const size_t minSize = request->GetStartIndex() + blockCount;
                if (Blocks.size() < minSize) {
                    Blocks.resize(minSize, TString(4_KB, 0));
                }
                ui64 offset = 0;
                for (ui32 i = request->GetStartIndex(); i < minSize; ++i) {
                    Blocks[i] =
                        result.Data.substr(offset, request->GetBlockSize());
                    offset += request->GetBlockSize();
                }

                responseBytes = serializer->Serialize(
                    req->ResponseBuffer,
                    TBlockStoreProtocol::WriteDeviceBlocksResponse,
                    response,
                    TContIOVector(nullptr, 0));

                break;
            }

            case TBlockStoreProtocol::ZeroDeviceBlocksRequest: {
                NProto::TZeroDeviceBlocksResponse response;
                using TProto = NProto::TZeroDeviceBlocksRequest;
                auto* request = static_cast<TProto*>(result.Proto.get());
                const auto blockCount = request->GetBlocksCount();
                const size_t minSize = request->GetStartIndex() + blockCount;
                if (Blocks.size() < minSize) {
                    Blocks.resize(minSize, TString(4_KB, 0));
                }
                ui64 offset = 0;
                for (ui32 i = request->GetStartIndex(); i < minSize; ++i) {
                    Blocks[i] = TString(4_KB, 0);
                    offset += request->GetBlockSize();
                }

                responseBytes = serializer->Serialize(
                    req->ResponseBuffer,
                    TBlockStoreProtocol::ZeroDeviceBlocksResponse,
                    response,
                    TContIOVector(nullptr, 0));

                break;
            }

            default: {
                Y_VERIFY(false);
            }
        }

        Handler->HandleResponse(std::move(req), 0, responseBytes);
    }

    void FreeRequest(NRdma::TClientRequestPtr req) override
    {
        delete[] req->RequestBuffer.data();
        delete[] req->ResponseBuffer.data();
    }
};

////////////////////////////////////////////////////////////////////////////////

TString MakeKey(const TString& host, ui32 port)
{
    return Sprintf("%s:%u", host.c_str(), port);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

NThreading::TFuture<NRdma::IClientEndpointPtr> TRdmaClientTest::StartEndpoint(
    TString host,
    ui32 port,
    NRdma::IClientHandlerPtr handler)
{
    auto& ep = Endpoints[MakeKey(host, port)];
    if (!ep.Endpoint) {
        ep.Endpoint = std::make_shared<TRdmaEndpointImpl>(handler);
        ep.Promise = NThreading::NewPromise<NRdma::IClientEndpointPtr>();
    }
    return ep.Promise;
}

void TRdmaClientTest::InjectErrors(
    NProto::TError allocationError,
    NProto::TError responseError)
{
    for (auto& x: Endpoints) {
        auto& ep = static_cast<TRdmaEndpointImpl&>(*x.second.Endpoint);
        ep.AllocationError = allocationError;
        ep.ResponseError = responseError;
    }
}

void TRdmaClientTest::InitAllEndpoints()
{
    for (auto& x: Endpoints) {
        x.second.Promise.SetValue(x.second.Endpoint);
    }
}

void TRdmaClientTest::InitAllEndpointsWithError()
{
    for (auto& x: Endpoints) {
        x.second.Promise.SetException("init failure");
    }
}

}   // namespace NCloud::NBlockStore::NStorage
