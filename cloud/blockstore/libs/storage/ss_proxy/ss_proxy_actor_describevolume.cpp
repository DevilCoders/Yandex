#include "ss_proxy_actor.h"

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/volume_label.h>
#include <cloud/blockstore/libs/storage/ss_proxy/ss_proxy_events_private.h>

#include <cloud/storage/core/libs/common/helpers.h>

#include <ydb/core/tx/tx_proxy/proxy.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NSchemeShard;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration Timeout = TDuration::Seconds(20);

////////////////////////////////////////////////////////////////////////////////

class TDescribeVolumeActor final
    : public TActorBootstrapped<TDescribeVolumeActor>
{
private:
    const TRequestInfoPtr RequestInfo;

    const TStorageConfigPtr Config;
    const TString DiskId;

    bool FallbackRequest = false;

public:
    TDescribeVolumeActor(
        TRequestInfoPtr requestInfo,
        TStorageConfigPtr config,
        TString diskId);

    void Bootstrap(const TActorContext& ctx);

private:
    void DescribeVolume(const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvSSProxy::TEvDescribeVolumeResponse> response);

    bool CheckVolume(
        const TActorContext& ctx,
        const NKikimrBlockStore::TVolumeConfig& volumeConfig) const;

    TString GetFullPath() const;

private:
    STFUNC(StateWork);

    void HandleDescribeSchemeResponse(
        const TEvSSProxy::TEvDescribeSchemeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleWakeup(
        const NActors::TEvents::TEvWakeup::TPtr& ev,
        const NActors::TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TDescribeVolumeActor::TDescribeVolumeActor(
        TRequestInfoPtr requestInfo,
        TStorageConfigPtr config,
        TString diskId)
    : RequestInfo(std::move(requestInfo))
    , Config(std::move(config))
    , DiskId(std::move(diskId))
{
    ActivityType = TBlockStoreActivities::SS_PROXY;
}

void TDescribeVolumeActor::Bootstrap(const TActorContext& ctx)
{
    ctx.Schedule(Timeout, new TEvents::TEvWakeup());
    DescribeVolume(ctx);
    Become(&TThis::StateWork);
}

bool TDescribeVolumeActor::CheckVolume(
    const TActorContext& ctx,
    const NKikimrBlockStore::TVolumeConfig& volumeConfig) const
{
    ui64 blocksCount = 0;
    for (const auto& partition: volumeConfig.GetPartitions()) {
        blocksCount += partition.GetBlockCount();
    }

    if (!blocksCount || !volumeConfig.GetBlockSize()) {
        LOG_ERROR(ctx, TBlockStoreComponents::SS_PROXY,
            "Broken config for volume %s",
            GetFullPath().Quote().data());
        return false;
    }

    return true;
}

TString TDescribeVolumeActor::GetFullPath() const
{
    TStringBuilder fullPath;
    fullPath << Config->GetSchemeShardDir() << '/';

    if (!FallbackRequest) {
        fullPath << DiskIdToPathDeprecated(DiskId);
    } else {
        fullPath << DiskIdToPath(DiskId);
    }

    return fullPath;
}

void TDescribeVolumeActor::DescribeVolume(const TActorContext& ctx)
{
    auto request =
        std::make_unique<TEvSSProxy::TEvDescribeSchemeRequest>(GetFullPath());

    NCloud::Send(ctx, MakeSSProxyServiceId(), std::move(request));
}

void TDescribeVolumeActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvSSProxy::TEvDescribeVolumeResponse> response)
{
    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TDescribeVolumeActor::HandleDescribeSchemeResponse(
    const TEvSSProxy::TEvDescribeSchemeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto error = msg->GetError();    

    if (HasError(error)) {
        auto status =
            static_cast<NKikimrScheme::EStatus>(STATUS_FROM_CODE(error.GetCode()));
       
        // TODO: return E_NOT_FOUND instead of StatusPathDoesNotExist
        if (status == NKikimrScheme::StatusPathDoesNotExist) {
            if (!FallbackRequest) {
                FallbackRequest = true;
                DescribeVolume(ctx);
                return;
            }
        }

        ReplyAndDie(
            ctx,
            std::make_unique<TEvSSProxy::TEvDescribeVolumeResponse>(error));
        return;
    }

    const auto& pathDescription = msg->PathDescription;
    const auto pathType = pathDescription.GetSelf().GetPathType();

    if (pathType != NKikimrSchemeOp::EPathTypeBlockStoreVolume) {
        ReplyAndDie(
            ctx,
            std::make_unique<TEvSSProxy::TEvDescribeVolumeResponse>(
                MakeError(
                    E_FAIL,
                    TStringBuilder() << "Described path is not a blockstore volume: "
                        << GetFullPath().Quote()
                )
            )
        );
        return;
    }

    const auto& volumeDescription =
        pathDescription.GetBlockStoreVolumeDescription();
    const auto& volumeConfig = volumeDescription.GetVolumeConfig();

    if (!CheckVolume(ctx, volumeConfig)) {
        // re-try request until get correct config or timeout
        DescribeVolume(ctx);
        return;
    }

    auto response = std::make_unique<TEvSSProxy::TEvDescribeVolumeResponse>(
        msg->Path,
        pathDescription);

    ReplyAndDie(ctx, std::move(response));
}

void TDescribeVolumeActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_ERROR(ctx, TBlockStoreComponents::SS_PROXY,
        "Describe request timed out for volume %s",
        GetFullPath().Quote().data());

    auto response = std::make_unique<TEvSSProxy::TEvDescribeVolumeResponse>(
        MakeError(
            E_TIMEOUT,
            TStringBuilder() << "DescribeVolume timeout for volume: "
                << GetFullPath().Quote()
        )
    );

    ReplyAndDie(ctx, std::move(response));
}

STFUNC(TDescribeVolumeActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvDescribeSchemeResponse, HandleDescribeSchemeResponse);

        HFunc(TEvents::TEvWakeup, HandleWakeup);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SS_PROXY);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TSSProxyActor::HandleDescribeVolume(
    const TEvSSProxy::TEvDescribeVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    NCloud::Register<TDescribeVolumeActor>(
        ctx,
        std::move(requestInfo),
        Config,
        msg->DiskId);
}

}   // namespace NCloud::NBlockStore::NStorage
