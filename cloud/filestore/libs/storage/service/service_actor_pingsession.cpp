#include "service_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

void TStorageServiceActor::HandlePingSession(
    const TEvService::TEvPingSessionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto& clientId = GetClientId(msg->Record);
    const auto& sessionId = GetSessionId(msg->Record);

    auto* session = State->FindSession(sessionId);
    if (!session || session->ClientId != clientId) {
        auto response = std::make_unique<TEvService::TEvPingSessionResponse>(
            ErrorInvalidSession(clientId, sessionId));

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    NCloud::Send(
        ctx,
        session->SessionActor,
        std::make_unique<TEvServicePrivate::TEvPingSession>());

    auto response = std::make_unique<TEvService::TEvPingSessionResponse>();
    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
