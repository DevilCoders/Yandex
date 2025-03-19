#include "hive_proxy_actor.h"

namespace NCloud::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

void THiveProxyActor::HandleCreateTablet(
    const TEvHiveProxy::TEvCreateTabletRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto key = std::make_pair(
        msg->Request.GetOwner(),
        msg->Request.GetOwnerIdx());

    HiveStates[msg->HiveId].CreateRequests[key].emplace_back(
        TRequestInfo(ev->Sender, ev->Cookie),
        false);

    auto hiveRequest = std::make_unique<TEvHive::TEvCreateTablet>();

    hiveRequest->Record = msg->Request;

    ClientCache->Send(ctx, msg->HiveId, hiveRequest.release(), msg->HiveId);
}

void THiveProxyActor::HandleLookupTablet(
    const TEvHiveProxy::TEvLookupTabletRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto key = std::make_pair(msg->Owner, msg->OwnerIdx);

    HiveStates[msg->HiveId].CreateRequests[key].emplace_back(
        TRequestInfo(ev->Sender, ev->Cookie),
        true);

    auto hiveRequest = std::make_unique<TEvHive::TEvLookupTablet>(
        msg->Owner, msg->OwnerIdx);

    ClientCache->Send(ctx, msg->HiveId, hiveRequest.release(), msg->HiveId);
}

void THiveProxyActor::HandleCreateTabletReply(
    const TEvHive::TEvCreateTabletReply::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto hiveId = ev->Cookie;
    auto& state = HiveStates[hiveId];

    const auto key = std::make_pair(
         msg->Record.GetOwner(),
         msg->Record.GetOwnerIdx());

    auto status = msg->Record.GetStatus();
    bool isOk = (status == NKikimrProto::OK) || (status == NKikimrProto::ALREADY);

    const auto error = MakeError(isOk ? S_OK : E_NOT_FOUND);

    const auto tabletId = msg->Record.GetTabletID();

    auto it = state.CreateRequests.find(key);

    if (it == state.CreateRequests.end()) {
        return;
    }

    for (const auto& [request, isLookup] : it->second) {
        if (isLookup) {
            auto response = std::make_unique<TEvHiveProxy::TEvLookupTabletResponse>(
                error, tabletId);

            NCloud::ReplyNoTrace(ctx, request, std::move(response));

            continue;
        }

        auto response = std::make_unique<TEvHiveProxy::TEvCreateTabletResponse>(
            error, tabletId);

        NCloud::ReplyNoTrace(ctx, request, std::move(response));
    }

    state.CreateRequests.erase(it);
}

void THiveProxyActor::HandleTabletCreation(
    const TEvHive::TEvTabletCreationResult::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    LOG_DEBUG_S(ctx, LogComponent, ev->Get()->ToString());
}

}   // namespace NCloud::NStorage
