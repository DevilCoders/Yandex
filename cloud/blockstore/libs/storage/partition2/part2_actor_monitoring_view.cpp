#include "part2_actor.h"

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/monlib/service/pages/templates.h>

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore::NStorage::NPartition2 {

using namespace NKikimr;
using namespace NMonitoringUtils;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class THttpReadBlockActor final
    : public TActorBootstrapped<THttpReadBlockActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const bool DataDumpAllowed;
    const TActorId Tablet;
    const ui32 BlockIndex;
    const ui64 CommitId;
    const bool Binary;

    TGuardedBuffer<TString> BufferHolder;

public:
    THttpReadBlockActor(
        TRequestInfoPtr requestInfo,
        bool dataDumpAllowed,
        const TActorId& tablet,
        ui32 blockSize,
        ui32 blockIndex,
        ui64 commitId,
        bool binary);

    void Bootstrap(const TActorContext& ctx);

private:
    void ReadBlocks(const TActorContext& ctx);

    void ReplyAndDie(const TActorContext& ctx, const TString& buffer);
    void ReplyAndDie(const TActorContext& ctx, const NProto::TError& error);

    void SendBinaryResponse(
        const TActorContext& ctx,
        const TString& buffer);

    void SendHttpResponse(
        const TActorContext& ctx,
        const NProto::TError& error,
        const TString& buffer);

private:
    STFUNC(StateWork);

    void HandleReadBlockResponse(
        const TEvService::TEvReadBlocksLocalResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

THttpReadBlockActor::THttpReadBlockActor(
        TRequestInfoPtr requestInfo,
        bool dataDumpAllowed,
        const TActorId& tablet,
        ui32 blockSize,
        ui32 blockIndex,
        ui64 commitId,
        bool binary)
    : RequestInfo(std::move(requestInfo))
    , DataDumpAllowed(dataDumpAllowed)
    , Tablet(tablet)
    , BlockIndex(blockIndex)
    , CommitId(commitId)
    , Binary(binary)
    , BufferHolder(TString::Uninitialized(blockSize))
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void THttpReadBlockActor::Bootstrap(const TActorContext& ctx)
{
    LWPROBE(
        RequestReceived_PartitionWorker,
        "HttpInfo",
        GetRequestId(RequestInfo->TraceId));

    ReadBlocks(ctx);

    Become(&TThis::StateWork);
}

void THttpReadBlockActor::ReadBlocks(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvService::TEvReadBlocksLocalRequest>();
    request->Record.SetStartIndex(BlockIndex);
    request->Record.SetBlocksCount(1);
    request->Record.Sglist = BufferHolder.GetGuardedSgList();
    request->Record.CommitId = CommitId;
    request->Record.BlockSize = BufferHolder.Get().size();

    auto traceId = RequestInfo->TraceId.Clone();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    NCloud::Send(
        ctx,
        Tablet,
        std::move(request),
        0,  // cookie
        std::move(traceId));
}

void THttpReadBlockActor::ReplyAndDie(
    const TActorContext& ctx,
    const TString& buffer)
{
    if (Binary) {
        SendBinaryResponse(ctx, buffer);
    } else {
        SendHttpResponse(ctx, {}, buffer);
    }

    Die(ctx);
}

void THttpReadBlockActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    SendHttpResponse(ctx, error, {});
    Die(ctx);
}

void THttpReadBlockActor::SendBinaryResponse(
    const TActorContext& ctx,
    const TString& buffer)
{
    TStringStream out;

    out << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: application/octet-stream\r\n"
        << "Content-Disposition: attachment; filename=\"Block_"
        << BlockIndex << "_" << CommitId << "\"\r\n"
        << "Connection: close\r\n"
        << "\r\n";
    out << buffer;

    auto response = std::make_unique<NMon::TEvRemoteBinaryInfoRes>(out.Str());

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    LWPROBE(
        ResponseSent_Partition,
        "HttpInfo",
        GetRequestId(RequestInfo->TraceId));

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
}

void THttpReadBlockActor::SendHttpResponse(
    const TActorContext& ctx,
    const NProto::TError& error,
    const TString& buffer)
{
    TStringStream out;

    HTML(out) {
        DIV_CLASS("panel panel-info") {
            DIV_CLASS("panel-heading") {
                out << "Block Content";
            }
            DIV_CLASS("panel-body") {
                DIV() {
                    out << "BlockIndex: ";
                    STRONG() {
                        out << BlockIndex;
                    }
                }
                DIV() {
                    out << "CommitId: ";
                    STRONG() {
                        out << CommitId;
                    }
                }
                DIV() {
                    out << "Status: ";
                    STRONG() {
                        out << FormatError(error);
                    }
                }
                if (buffer) {
                    if (DataDumpAllowed) {
                        DumpBlockContent(out, buffer);
                    } else {
                        DumpDataHash(out, buffer);
                    }
                }
            }
        }
    }

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    LWPROBE(
        ResponseSent_Partition,
        "HttpInfo",
        GetRequestId(RequestInfo->TraceId));

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void THttpReadBlockActor::HandleReadBlockResponse(
    const TEvService::TEvReadBlocksLocalResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (FAILED(msg->GetStatus())) {
        ReplyAndDie(ctx, msg->GetError());
        return;
    }

    ReplyAndDie(ctx, BufferHolder.Extract());
}

void THttpReadBlockActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    ReplyAndDie(ctx, MakeError(E_REJECTED, "Tablet is dead"));
}

STFUNC(THttpReadBlockActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvService::TEvReadBlocksLocalResponse, HandleReadBlockResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleHttpInfo_View(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TCgiParameters& params)
{
    if (params.Get("block") && params.Has("commitid")) {
        ui32 blockIndex = 0;
        ui64 commitId = 0;

        if (TryFromString(params.Get("block"), blockIndex) &&
            TryFromString(params.Get("commitid"), commitId))
        {
            NCloud::Register<THttpReadBlockActor>(
                ctx,
                std::move(requestInfo),
                Config->GetUserDataDebugDumpAllowed(),
                SelfId(),
                State->GetBlockSize(),
                blockIndex,
                commitId,
                params.Has("binary"));
        } else {
            auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(
                "invalid index specified");

            BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

            LWPROBE(
                ResponseSent_Partition,
                "HttpInfo",
                GetRequestId(requestInfo->TraceId));

            NCloud::Reply(ctx, *requestInfo, std::move(response));
        }
        return;
    }

    using namespace NMonitoringUtils;

    TStringStream out;
    DumpDefaultHeader(out, *Info(), SelfId().NodeId(), *DiagnosticsConfig);

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());

    BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

    LWPROBE(
        ResponseSent_Partition,
        "HttpInfo",
        GetRequestId(requestInfo->TraceId));

    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition2
