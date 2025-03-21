#include "encryption_client.h"

#include "encryptor.h"

#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/media.h>
#include <cloud/storage/core/libs/common/verify.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/threading/future/future.h>

namespace NCloud::NBlockStore::NClient {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

void CopyResponseWithoutData(
    NProto::TReadBlocksResponse& dst,
    const NProto::TReadBlocksResponse& src)
{
    *dst.MutableError() = src.GetError();
    *dst.MutableTrace() = src.GetTrace();
    *dst.MutableUnencryptedBlockMask() = src.GetUnencryptedBlockMask();
}

void CopyRequestWithoutData(
    NProto::TWriteBlocksRequest& dst,
    const NProto::TWriteBlocksRequest& src)
{
    *dst.MutableHeaders() = src.GetHeaders();
    *dst.MutableDiskId() = src.GetDiskId();
    dst.SetStartIndex(src.GetStartIndex());
    dst.SetFlags(src.GetFlags());
    *dst.MutableSessionId() = src.GetSessionId();
}

void CopyLocalRequestWithoutData(
    NProto::TReadBlocksLocalRequest& dst,
    const NProto::TReadBlocksLocalRequest& src)
{
    *dst.MutableHeaders() = src.GetHeaders();
    *dst.MutableDiskId() = src.GetDiskId();
    dst.SetStartIndex(src.GetStartIndex());
    dst.SetBlocksCount(src.GetBlocksCount());
    dst.SetFlags(src.GetFlags());
    *dst.MutableCheckpointId() = src.GetCheckpointId();
    *dst.MutableSessionId() = src.GetSessionId();
    dst.BlockSize = src.BlockSize;
    dst.CommitId = src.CommitId;
}

void CopyLocalRequestWithoutData(
    NProto::TWriteBlocksLocalRequest& dst,
    const NProto::TWriteBlocksLocalRequest& src)
{
    CopyRequestWithoutData(dst, src);
    dst.BlocksCount = src.BlocksCount;
    dst.BlockSize = src.BlockSize;
}

////////////////////////////////////////////////////////////////////////////////

template <typename TResponse>
TFuture<TResponse> FutureErrorResponse(ui32 code, TString message)
{
    return MakeFuture(ErrorResponse<TResponse>(code, std::move(message)));
}

////////////////////////////////////////////////////////////////////////////////

TStorageBuffer AllocateStorageBuffer(
    IBlockStore& client,
    size_t bytesCount)
{
    auto buffer = client.AllocateBuffer(bytesCount);

    if (!buffer) {
        buffer = std::shared_ptr<char>(
            new char[bytesCount],
            std::default_delete<char[]>());
    }
    return buffer;
}

////////////////////////////////////////////////////////////////////////////////

bool GetBitValue(const TString& bitmask, size_t bitNum)
{
    size_t byte = bitNum / 8;
    if (byte >= bitmask.size()) {
        return false;
    }

    size_t bit = bitNum % 8;
    return (bitmask[byte] & (1 << bit)) != 0;
}

////////////////////////////////////////////////////////////////////////////////

void ZeroUnencryptedBlocksInResponse(NProto::TReadBlocksResponse& response)
{
    const auto& unencryptedBlockMask = response.GetUnencryptedBlockMask();
    auto& buffers = *response.MutableBlocks()->MutableBuffers();

    for (int i = 0; i < buffers.size(); ++i) {
        auto& buffer = buffers[i];
        if (GetBitValue(unencryptedBlockMask, i)) {
            buffer.clear();
        }
    }
}

void ZeroUnencryptedBlocksInLocalResponse(
    const TSgList& sglist,
    const TString& unencryptedBlockMask)
{
    for (size_t i = 0; i < sglist.size(); ++i) {
        auto& buffer = sglist[i];
        if (GetBitValue(unencryptedBlockMask, i)) {
            memset(const_cast<char*>(buffer.Data()), 0, buffer.Size());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

class TClientWrapper
    : public IBlockStore
{
protected:
    IBlockStorePtr Client;

public:
    TClientWrapper(IBlockStorePtr client)
        : Client(std::move(client))
    {}

    void Start() override
    {
        Client->Start();
    }

    void Stop() override
    {
        Client->Stop();
    }

    TStorageBuffer AllocateBuffer(size_t bytesCount) override
    {
        return Client->AllocateBuffer(bytesCount);
    }

#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr ctx,                                                   \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        return Client->name(std::move(ctx), std::move(request));               \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_IMPLEMENT_METHOD)

#undef BLOCKSTORE_IMPLEMENT_METHOD
};

////////////////////////////////////////////////////////////////////////////////

class TEncryptionClient final
    : public TClientWrapper
    , public std::enable_shared_from_this<TEncryptionClient>
{
private:
    const IEncryptorPtr Encryptor;
    const TString EncryptionKeyHash;

    NProto::EStorageMediaKind StorageMediaKind = NProto::STORAGE_MEDIA_DEFAULT;
    ui32 BlockSize = 0;
    TString ZeroBlock;

    TLog Log;

public:
    TEncryptionClient(
            IBlockStorePtr client,
            ILoggingServicePtr logging,
            IEncryptorPtr encryptor,
            TString encryptionKeyHash)
        : TClientWrapper(std::move(client))
        , Encryptor(std::move(encryptor))
        , EncryptionKeyHash(std::move(encryptionKeyHash))
        , Log(logging->CreateLog("BLOCKSTORE_CLIENT"))
    {}

    TFuture<NProto::TMountVolumeResponse> MountVolume(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TMountVolumeRequest> request) override;

    TFuture<NProto::TReadBlocksResponse> ReadBlocks(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TReadBlocksRequest> request) override;

    TFuture<NProto::TWriteBlocksResponse> WriteBlocks(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TWriteBlocksRequest> request) override;

    TFuture<NProto::TReadBlocksLocalResponse> ReadBlocksLocal(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TReadBlocksLocalRequest> request) override;

    TFuture<NProto::TWriteBlocksLocalResponse> WriteBlocksLocal(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TWriteBlocksLocalRequest> request) override;

    TFuture<NProto::TZeroBlocksResponse> ZeroBlocks(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TZeroBlocksRequest> request) override;

private:
    bool Encrypt(
        const TSgList& src,
        const TSgList& dst,
        ui64 startIndex);

    bool Decrypt(
        const TSgList& src,
        const TSgList& dst,
        ui64 startIndex,
        const TString& unencryptedBlockMask);

    void HandleMountVolumeResponse(
        const NProto::TMountVolumeResponse& response);

    NProto::TReadBlocksResponse HandleReadBlocksResponse(
        const NProto::TReadBlocksResponse& response,
        ui64 startIndex,
        ui32 blocksCount);

    NProto::TReadBlocksLocalResponse HandleReadBlocksLocalResponse(
        NProto::TReadBlocksLocalResponse response,
        const TSgList& encryptedSglist,
        const NProto::TReadBlocksLocalRequest& request);
};

////////////////////////////////////////////////////////////////////////////////

TFuture<NProto::TMountVolumeResponse> TEncryptionClient::MountVolume(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TMountVolumeRequest> request)
{
    if (request->GetEncryptionKeyHash()) {
        return FutureErrorResponse<NProto::TMountVolumeResponse>(
            E_INVALID_STATE,
            "More than one encryption layer on data path");
    }

    request->SetEncryptionKeyHash(EncryptionKeyHash);

    auto future = Client->MountVolume(
        std::move(callContext),
        std::move(request));

    auto weakPtr = weak_from_this();

    return future.Apply([weakPtr = std::move(weakPtr)] (const auto& f) {
        const auto& response = f.GetValue();
        if (HasError(response)) {
            return response;
        }

        auto ptr = weakPtr.lock();
        if (!ptr) {
            return static_cast<NProto::TMountVolumeResponse>(TErrorResponse(
                E_REJECTED,
                "Encryption client is destroyed"));
        }

        ptr->HandleMountVolumeResponse(response);
        return response;
    });
}

void TEncryptionClient::HandleMountVolumeResponse(
    const NProto::TMountVolumeResponse& response)
{
    if (BlockSize == 0) {
        StorageMediaKind = response.GetVolume().GetStorageMediaKind();
        BlockSize = response.GetVolume().GetBlockSize();
        ZeroBlock = TString(BlockSize, 0);
    }
}

TFuture<NProto::TReadBlocksResponse> TEncryptionClient::ReadBlocks(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TReadBlocksRequest> request)
{
    if (request->GetBlocksCount() == 0 || BlockSize == 0) {
        return FutureErrorResponse<NProto::TReadBlocksResponse>(
            E_ARGUMENT,
            "Request size should not be zero");
    }

    auto startIndex = request->GetStartIndex();
    auto blocksCount = request->GetBlocksCount();

    auto future = Client->ReadBlocks(
        std::move(callContext),
        std::move(request));

    auto weakPtr = weak_from_this();

    return future.Apply([
        weakPtr = std::move(weakPtr),
        startIndex,
        blocksCount] (const auto& f)
    {
        const auto& response = f.GetValue();
        if (HasError(response)) {
            return response;
        }

        auto ptr = weakPtr.lock();
        if (!ptr) {
            return static_cast<NProto::TReadBlocksResponse>(TErrorResponse(
                E_REJECTED,
                "Encryption client is destroyed"));
        }

        return ptr->HandleReadBlocksResponse(
            response,
            startIndex,
            blocksCount);
    });
}

NProto::TReadBlocksResponse TEncryptionClient::HandleReadBlocksResponse(
    const NProto::TReadBlocksResponse& response,
    ui64 startIndex,
    ui32 blocksCount)
{
    NProto::TReadBlocksResponse decryptedResponse;
    CopyResponseWithoutData(decryptedResponse, response);

    auto decryptedSglist = ResizeIOVector(
        *decryptedResponse.MutableBlocks(),
        blocksCount,
        BlockSize);

    auto sgListOrError = GetSgList(response, BlockSize);
    if (HasError(sgListOrError)) {
        return TErrorResponse(sgListOrError.GetError());
    }

    bool res = Decrypt(
        sgListOrError.GetResult(),
        decryptedSglist,
        startIndex,
        response.GetUnencryptedBlockMask());

    if (!res) {
        return ErrorResponse<NProto::TReadBlocksResponse>(
            E_INVALID_STATE,
            "Failed to decrypt blocks");
    }

    if (IsDiskRegistryMediaKind(StorageMediaKind)) {
        ZeroUnencryptedBlocksInResponse(decryptedResponse);
    }

    return decryptedResponse;
}

TFuture<NProto::TWriteBlocksResponse> TEncryptionClient::WriteBlocks(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TWriteBlocksRequest> request)
{
    if (request->GetBlocks().GetBuffers().size() == 0 || BlockSize == 0) {
        return FutureErrorResponse<NProto::TWriteBlocksResponse>(
            E_ARGUMENT,
            "Request size should not be zero");
    }

    auto sgListOrError = SgListNormalize(GetSgList(*request), BlockSize);
    if (HasError(sgListOrError)) {
        return MakeFuture<NProto::TWriteBlocksResponse>(
            TErrorResponse(sgListOrError.GetError()));
    }
    auto sglist = sgListOrError.ExtractResult();

    auto encryptedRequest = std::make_shared<NProto::TWriteBlocksRequest>();
    CopyRequestWithoutData(*encryptedRequest, *request);

    auto encryptedSglist = ResizeIOVector(
        *encryptedRequest->MutableBlocks(),
        sglist.size(),
        BlockSize);

    bool res = Encrypt(
        sglist,
        encryptedSglist,
        request->GetStartIndex());

    if (!res) {
        return FutureErrorResponse<NProto::TWriteBlocksResponse>(
            E_INVALID_STATE,
            "Failed to encrypt blocks");
    }

    return Client->WriteBlocks(
        std::move(callContext),
        std::move(encryptedRequest));
}

TFuture<NProto::TReadBlocksLocalResponse> TEncryptionClient::ReadBlocksLocal(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TReadBlocksLocalRequest> request)
{
    if (request->GetBlocksCount() == 0 || request->BlockSize == 0) {
        return FutureErrorResponse<NProto::TReadBlocksLocalResponse>(
            E_ARGUMENT,
            "Request size should not be zero");
    }

    ui64 bufferSize = static_cast<ui64>(request->GetBlocksCount()) *
        request->BlockSize;
    auto buffer = AllocateStorageBuffer(*Client, bufferSize);
    auto sgListOrError = SgListNormalize(
        { buffer.get(), bufferSize },
        request->BlockSize);

    if (HasError(sgListOrError)) {
        return MakeFuture<NProto::TReadBlocksLocalResponse>(
            TErrorResponse(sgListOrError.GetError()));
    }

    TGuardedSgList guardedSgList(sgListOrError.ExtractResult());

    auto encryptedRequest = std::make_shared<NProto::TReadBlocksLocalRequest>();
    CopyLocalRequestWithoutData(*encryptedRequest, *request);
    encryptedRequest->Sglist = guardedSgList;

    auto future = Client->ReadBlocksLocal(
        std::move(callContext),
        std::move(encryptedRequest));

    auto weakPtr = weak_from_this();

    return future.Apply([
        weakPtr = std::move(weakPtr),
        request = std::move(request),
        sgList = std::move(guardedSgList),
        buf = std::move(buffer)] (const auto& f) mutable
    {
        auto encryptedSglist = sgList.Acquire().Get();
        sgList.Destroy();

        auto response = f.GetValue();
        if (HasError(response)) {
            return response;
        }

        auto ptr = weakPtr.lock();
        if (!ptr) {
            return static_cast<NProto::TReadBlocksResponse>(TErrorResponse(
                E_REJECTED,
                "Encryption client is destroyed"));
        }

        Y_UNUSED(buf);
        return ptr->HandleReadBlocksLocalResponse(
            std::move(response),
            encryptedSglist,
            *request);
    });
}

NProto::TReadBlocksLocalResponse TEncryptionClient::HandleReadBlocksLocalResponse(
    NProto::TReadBlocksLocalResponse response,
    const TSgList& encryptedSglist,
    const NProto::TReadBlocksLocalRequest& request)
{
    auto guard = request.Sglist.Acquire();
    if (!guard) {
        return ErrorResponse<NProto::TReadBlocksLocalResponse>(
            E_CANCELLED,
            "Local request was cancelled");
    }

    auto sgListOrError = SgListNormalize(guard.Get(), request.BlockSize);
    if (HasError(sgListOrError)) {
        return TErrorResponse(sgListOrError.GetError());
    }

    bool res = Decrypt(
        encryptedSglist,
        sgListOrError.GetResult(),
        request.GetStartIndex(),
        response.GetUnencryptedBlockMask());

    if (!res) {
        return ErrorResponse<NProto::TReadBlocksLocalResponse>(
            E_INVALID_STATE,
            "Failed to decrypt blocks");
    }

    if (IsDiskRegistryMediaKind(StorageMediaKind)) {
        ZeroUnencryptedBlocksInLocalResponse(
            sgListOrError.GetResult(),
            response.GetUnencryptedBlockMask());
    }

    return response;
}

TFuture<NProto::TWriteBlocksLocalResponse> TEncryptionClient::WriteBlocksLocal(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TWriteBlocksLocalRequest> request)
{
    if (request->BlocksCount == 0 || request->BlockSize == 0) {
        return FutureErrorResponse<NProto::TWriteBlocksLocalResponse>(
            E_ARGUMENT,
            "Request size should not be zero");
    }

    auto guard = request->Sglist.Acquire();
    if (!guard) {
        return FutureErrorResponse<NProto::TWriteBlocksLocalResponse>(
            E_CANCELLED,
            "Local request was cancelled");
    }

    ui64 bufferSize = static_cast<ui64>(request->BlocksCount) *
        request->BlockSize;
    auto buffer = AllocateStorageBuffer(*Client, bufferSize);

    TSgList encryptedSglist;
    {
        auto sgListOrError = SgListNormalize(
            { buffer.get(), bufferSize },
            request->BlockSize);

        if (HasError(sgListOrError)) {
            return MakeFuture<NProto::TWriteBlocksLocalResponse>(
                TErrorResponse(sgListOrError.GetError()));
        }
        encryptedSglist = sgListOrError.ExtractResult();
    }

    TSgList srcSglist;
    {
        auto sgListOrError = SgListNormalize(guard.Get(), request->BlockSize);
        if (HasError(sgListOrError)) {
            return MakeFuture<NProto::TWriteBlocksLocalResponse>(
                TErrorResponse(sgListOrError.GetError()));
        }
        srcSglist = sgListOrError.ExtractResult();
    }

    bool res = Encrypt(
        srcSglist,
        encryptedSglist,
        request->GetStartIndex());

    if (!res) {
        return FutureErrorResponse<NProto::TWriteBlocksLocalResponse>(
            E_INVALID_STATE,
            "Failed to encrypt blocks");
    }

    TGuardedSgList guardedSgList(std::move(encryptedSglist));

    auto encryptedRequest = std::make_shared<NProto::TWriteBlocksLocalRequest>();
    CopyLocalRequestWithoutData(*encryptedRequest, *request);
    encryptedRequest->Sglist = guardedSgList;

    auto future = Client->WriteBlocksLocal(
        std::move(callContext),
        std::move(encryptedRequest));

    return future.Apply([
        sgList = std::move(guardedSgList),
        buf = std::move(buffer)] (const auto& f) mutable
    {
        sgList.Destroy();
        buf.reset();
        return f;
    });
}

TFuture<NProto::TZeroBlocksResponse> TEncryptionClient::ZeroBlocks(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TZeroBlocksRequest> request)
{
    if (request->GetBlocksCount() == 0 || BlockSize == 0) {
        return FutureErrorResponse<NProto::TZeroBlocksResponse>(
            E_ARGUMENT,
            "Request size should not be zero");
    }

    STORAGE_VERIFY(
        BlockSize <= ZeroBlock.Size(),
        TWellKnownEntityTypes::DISK,
        request->GetDiskId());
    TBlockDataRef zeroDataRef(ZeroBlock.Data(), BlockSize);
    TSgList zeroSgList(request->GetBlocksCount(), zeroDataRef);
    TGuardedSgList guardedSgList(std::move(zeroSgList));

    auto writeRequest = std::make_shared<NProto::TWriteBlocksLocalRequest>();
    writeRequest->MutableHeaders()->CopyFrom(request->GetHeaders());
    writeRequest->SetDiskId(request->GetDiskId());
    writeRequest->SetStartIndex(request->GetStartIndex());
    writeRequest->SetFlags(request->GetFlags());
    writeRequest->SetSessionId(request->GetSessionId());
    writeRequest->BlocksCount = request->GetBlocksCount();
    writeRequest->BlockSize = BlockSize;
    writeRequest->Sglist = guardedSgList;

    auto future = WriteBlocksLocal(
        std::move(callContext),
        std::move(writeRequest));

    return future.Apply([
        sgList = std::move(guardedSgList)] (const auto& f) mutable
    {
        sgList.Destroy();

        const auto& response = f.GetValue();

        NProto::TZeroBlocksResponse zeroResponse;
        zeroResponse.MutableError()->CopyFrom(response.GetError());
        zeroResponse.MutableTrace()->CopyFrom(response.GetTrace());
        return zeroResponse;
    });
}

bool TEncryptionClient::Encrypt(
    const TSgList& src,
    const TSgList& dst,
    ui64 startIndex)
{
    Y_VERIFY_DEBUG(dst.size() >= src.size());
    if (dst.size() < src.size()) {
        return false;
    }

    for (size_t i = 0; i < src.size(); ++i) {
        if (!Encryptor->Encrypt(src[i], dst[i], startIndex + i)) {
            return false;
        }
    }

    return true;
}

bool TEncryptionClient::Decrypt(
    const TSgList& src,
    const TSgList& dst,
    ui64 startIndex,
    const TString& unencryptedBlockMask)
{
    Y_VERIFY_DEBUG(dst.size() == src.size());
    if (dst.size() != src.size()) {
        return false;
    }

    for (size_t i = 0; i < src.size(); ++i) {
        bool encrypted = !GetBitValue(unencryptedBlockMask, i);

        if (encrypted) {
            if (!Encryptor->Decrypt(src[i], dst[i], startIndex + i)) {
                return false;
            }
        } else {
            Y_VERIFY_DEBUG(dst[i].Size() == src[i].Size());
            if (dst[i].Size() != src[i].Size()) {
                return false;
            }
            auto* dstPtr = const_cast<char*>(dst[i].Data());

            if (src[i].Data()) {
                memcpy(dstPtr, src[i].Data(), src[i].Size());
            } else {
                memset(dstPtr, 0, src[i].Size());
            }
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

class TSnapshotEncryptionClient final
    : public TClientWrapper
{
private:
    const TString EncryptionKeyHash;

    TLog Log;

public:
    TSnapshotEncryptionClient(
            IBlockStorePtr client,
            ILoggingServicePtr logging,
            TString encryptionKeyHash)
        : TClientWrapper(std::move(client))
        , EncryptionKeyHash(std::move(encryptionKeyHash))
        , Log(logging->CreateLog("BLOCKSTORE_CLIENT"))
    {}

    TFuture<NProto::TMountVolumeResponse> MountVolume(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TMountVolumeRequest> request) override;

    TFuture<NProto::TReadBlocksResponse> ReadBlocks(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TReadBlocksRequest> request) override;

    TFuture<NProto::TReadBlocksLocalResponse> ReadBlocksLocal(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TReadBlocksLocalRequest> request) override;

    TFuture<NProto::TZeroBlocksResponse> ZeroBlocks(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TZeroBlocksRequest> request) override;

private:
    static NProto::TReadBlocksResponse HandleReadBlocksResponse(
        NProto::TReadBlocksResponse response);

    static NProto::TReadBlocksLocalResponse HandleReadBlocksLocalResponse(
        NProto::TReadBlocksLocalResponse response,
        const TGuardedSgList& sgList,
        ui32 blockSize);
};

////////////////////////////////////////////////////////////////////////////////

TFuture<NProto::TMountVolumeResponse> TSnapshotEncryptionClient::MountVolume(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TMountVolumeRequest> request)
{
    if (request->GetEncryptionKeyHash()) {
        return FutureErrorResponse<NProto::TMountVolumeResponse>(
            E_INVALID_STATE,
            "More than one encryption layer on data path");
    }

    request->SetEncryptionKeyHash(EncryptionKeyHash);

    return Client->MountVolume(
        std::move(callContext),
        std::move(request));
}

TFuture<NProto::TReadBlocksResponse> TSnapshotEncryptionClient::ReadBlocks(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TReadBlocksRequest> request)
{
    auto future = Client->ReadBlocks(
        std::move(callContext),
        std::move(request));

    return future.Apply([] (auto f) {
        auto response = ExtractResponse(f);
        return HandleReadBlocksResponse(std::move(response));
    });
}

NProto::TReadBlocksResponse TSnapshotEncryptionClient::HandleReadBlocksResponse(
    NProto::TReadBlocksResponse response)
{
    if (HasError(response)) {
        return response;
    }

    ZeroUnencryptedBlocksInResponse(response);
    return response;
}

TFuture<NProto::TReadBlocksLocalResponse> TSnapshotEncryptionClient::ReadBlocksLocal(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TReadBlocksLocalRequest> request)
{
    auto requestSglist = request->Sglist;
    auto blockSize = request->BlockSize;

    auto future = Client->ReadBlocksLocal(
        std::move(callContext),
        std::move(request));

    return future.Apply([sgList = std::move(requestSglist), blockSize] (auto f)
    {
        auto response = ExtractResponse(f);
        return HandleReadBlocksLocalResponse(
            std::move(response),
            sgList,
            blockSize);
    });
}

NProto::TReadBlocksLocalResponse TSnapshotEncryptionClient::HandleReadBlocksLocalResponse(
    NProto::TReadBlocksLocalResponse response,
    const TGuardedSgList& sgList,
    ui32 blockSize)
{
    if (HasError(response)) {
        return response;
    }

    auto guard = sgList.Acquire();
    if (!guard) {
        return ErrorResponse<NProto::TReadBlocksLocalResponse>(
            E_CANCELLED,
            "Local request was cancelled");
    }

    auto sgListOrError = SgListNormalize(guard.Get(), blockSize);
    if (HasError(sgListOrError)) {
        return TErrorResponse(sgListOrError.GetError());
    }

    ZeroUnencryptedBlocksInLocalResponse(
        sgListOrError.GetResult(),
        response.GetUnencryptedBlockMask());

    return response;
}

TFuture<NProto::TZeroBlocksResponse> TSnapshotEncryptionClient::ZeroBlocks(
    TCallContextPtr callContext,
    std::shared_ptr<NProto::TZeroBlocksRequest> request)
{
    Y_UNUSED(callContext);
    Y_UNUSED(request);

    return FutureErrorResponse<NProto::TZeroBlocksResponse>(
        E_NOT_IMPLEMENTED,
        "ZeroBlocks requests not supported by snapshot encryption client");
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IBlockStorePtr CreateEncryptionClient(
    IBlockStorePtr client,
    ILoggingServicePtr logging,
    IEncryptorPtr encryptor,
    TString encryptionKeyHash)
{
    return std::make_shared<TEncryptionClient>(
        std::move(client),
        std::move(logging),
        std::move(encryptor),
        std::move(encryptionKeyHash));
}

IBlockStorePtr CreateSnapshotEncryptionClient(
    IBlockStorePtr client,
    ILoggingServicePtr logging,
    TString encryptionKeyHash)
{
    return std::make_shared<TSnapshotEncryptionClient>(
        std::move(client),
        std::move(logging),
        std::move(encryptionKeyHash));
}

////////////////////////////////////////////////////////////////////////////////

TResultOrError<IBlockStorePtr> TryToCreateEncryptionClient(
    IBlockStorePtr client,
    ILoggingServicePtr logging,
    const NProto::TEncryptionSpec& encryptionSpec)
{
    if (encryptionSpec.HasKeyHash() &&
        !encryptionSpec.GetKeyHash().empty())
    {
        if (!encryptionSpec.GetDisableEncryption()) {
            return TErrorResponse(E_ARGUMENT, TStringBuilder()
                << "DisableEncryption should be true when KeyHash is passed");
        }

        return CreateSnapshotEncryptionClient(
            std::move(client),
            std::move(logging),
            encryptionSpec.GetKeyHash());
    }

    const auto& encryptionKey = encryptionSpec.GetKey();
    auto keyHashOrError = ComputeEncryptionKeyHash(encryptionKey);
    if (HasError(keyHashOrError)) {
        return keyHashOrError.GetError();
    }
    auto keyHash = keyHashOrError.ExtractResult();

    if (encryptionSpec.GetDisableEncryption()) {
        return CreateSnapshotEncryptionClient(
            std::move(client),
            std::move(logging),
            keyHash);
    }

    IEncryptorPtr encryptor;
    switch (encryptionKey.GetMode())
    {
        case NProto::NO_ENCRYPTION:
            Y_VERIFY(keyHash.empty());
            return client;

        case NProto::ENCRYPTION_AES_XTS: {
            auto encryptorOrError = CreateAesXtsEncryptor(
                encryptionKey.GetKeyPath());

            if (HasError(encryptorOrError)) {
                return encryptorOrError.GetError();
            }

            encryptor = encryptorOrError.ExtractResult();
            break;
        }

        case NProto::ENCRYPTION_TEST: {
            encryptor = CreateTestCaesarEncryptor(42);
            break;
        }

        default:
            return TErrorResponse(E_ARGUMENT, TStringBuilder()
                << "Unknown encryption mode: "
                << static_cast<int>(encryptionKey.GetMode()));
    }

    return CreateEncryptionClient(
        std::move(client),
        std::move(logging),
        std::move(encryptor),
        std::move(keyHash));
}

}   // namespace NCloud::NBlockStore::NClient
