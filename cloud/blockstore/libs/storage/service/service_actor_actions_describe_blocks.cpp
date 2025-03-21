#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <ydb/core/base/logoblob.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/json/json_reader.h>

#include <google/protobuf/util/json_util.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER)

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDescribeBlocksActionActor final
    : public TActorBootstrapped<TDescribeBlocksActionActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TString Input;

    TString DiskId;
    ui64 StartIndex = 0;
    ui32 BlocksCount = 0;
    TString CheckpointId;

public:
    TDescribeBlocksActionActor(TRequestInfoPtr requestInfo, TString input);

    void Bootstrap(const TActorContext& ctx);

private:
    void DescribeBlocks(const TActorContext& ctx);

    void HandleSuccess(const TActorContext& ctx, const TString& output);
    void HandleError(const TActorContext& ctx, const NProto::TError& error);

private:
    STFUNC(StateWork);

    void HandleDescribeBlocksResponse(
        const TEvVolume::TEvDescribeBlocksResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TDescribeBlocksActionActor::TDescribeBlocksActionActor(
        TRequestInfoPtr requestInfo,
        TString input)
    : RequestInfo(std::move(requestInfo))
    , Input(std::move(input))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TDescribeBlocksActionActor::Bootstrap(const TActorContext& ctx)
{
    NJson::TJsonValue input;
    if (!NJson::ReadJsonTree(Input, &input, false)) {
        HandleError(ctx, MakeError(E_ARGUMENT, "Input should be in JSON format"));
        return;
    }

    if (input.Has("DiskId")) {
        DiskId = input["DiskId"].GetStringRobust();
    }

    if (!DiskId) {
        HandleError(ctx, MakeError(E_ARGUMENT, "DiskId should be defined"));
        return;
    }

    if (input.Has("StartIndex")) {
        StartIndex = input["StartIndex"].GetUIntegerRobust();
    }

    if (input.Has("BlocksCount")) {
        BlocksCount = input["BlocksCount"].GetUIntegerRobust();
    }

    if (!BlocksCount) {
        HandleError(ctx, MakeError(E_ARGUMENT, "BlocksCount should be defined"));
        return;
    }

    if (input.Has("CheckpointId")) {
        CheckpointId = input["CheckpointId"].GetStringRobust();
    }

    DescribeBlocks(ctx);
    Become(&TThis::StateWork);
}

void TDescribeBlocksActionActor::DescribeBlocks(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvVolume::TEvDescribeBlocksRequest>(
        RequestInfo->CallContext);
    request->Record.SetDiskId(DiskId);
    request->Record.SetStartIndex(StartIndex);
    request->Record.SetBlocksCount(BlocksCount);
    request->Record.SetCheckpointId(CheckpointId);

    NCloud::Send(
        ctx,
        MakeVolumeProxyServiceId(),
        std::move(request),
        RequestInfo->Cookie,
        RequestInfo->TraceId.Clone());
}

void TDescribeBlocksActionActor::HandleSuccess(
    const TActorContext& ctx,
    const TString& output)
{
    auto response = std::make_unique<TEvService::TEvExecuteActionResponse>();
    response->Record.SetOutput(output);

    LWTRACK(
        ResponseSent_Service,
        RequestInfo->CallContext->LWOrbit,
        "ExecuteAction_describeblocks",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

void TDescribeBlocksActionActor::HandleError(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto response = std::make_unique<TEvService::TEvExecuteActionResponse>(error);

    LWTRACK(
        ResponseSent_Service,
        RequestInfo->CallContext->LWOrbit,
        "ExecuteAction_describeblocks",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TDescribeBlocksActionActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvVolume::TEvDescribeBlocksResponse, HandleDescribeBlocksResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

void TDescribeBlocksActionActor::HandleDescribeBlocksResponse(
    const TEvVolume::TEvDescribeBlocksResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();
    const auto& error = msg->GetError();

    if (FAILED(error.GetCode())) {
        HandleError(ctx, error);
        return;
    }

    // We don't need fresh blocks.
    msg->Record.ClearFreshBlockRanges();
    msg->Record.ClearTrace();

    TString response;
    google::protobuf::util::MessageToJsonString(msg->Record, &response);

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Execute action private API: describe blocks response: %s",
        response.data());

    HandleSuccess(ctx, response);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr TServiceActor::CreateDescribeBlocksActionActor(
    TRequestInfoPtr requestInfo,
    TString input)
{
    return std::make_unique<TDescribeBlocksActionActor>(
        std::move(requestInfo),
        std::move(input)
    );
}

}   // namespace NCloud::NBlockStore::NStorage
