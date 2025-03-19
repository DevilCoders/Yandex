#include "service_actor.h"

#include <cloud/filestore/libs/storage/api/tablet.h>
#include <cloud/filestore/libs/storage/api/tablet_proxy.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TStorageServiceActor::ForwardRequest(
    const TActorContext& ctx,
    const typename TMethod::TRequest::TPtr& ev)
{
    auto* msg = ev->Get();

    const auto& clientId = GetClientId(msg->Record);
    const auto& sessionId = GetSessionId(msg->Record);

    auto* session = State->FindSession(sessionId);
    if (!session || session->ClientId != clientId) {
        auto response = std::make_unique<typename TMethod::TResponse>(
            ErrorInvalidSession(clientId, sessionId));
        return NCloud::Reply(ctx, *ev, std::move(response));
    }

    // forward request through tablet proxy
    ctx.Send(ev->Forward(MakeIndexTabletProxyServiceId()));
}

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_FORWARD_REQUEST(name, ns)                                    \
    void TStorageServiceActor::Handle##name(                                   \
        const ns::TEv##name##Request::TPtr& ev,                                \
        const TActorContext& ctx)                                              \
    {                                                                          \
        ForwardRequest<ns::T##name##Method>(ctx, ev);                          \
    }                                                                          \
// FILESTORE_FORWARD_REQUEST

FILESTORE_SERVICE_REQUESTS_FWD(FILESTORE_FORWARD_REQUEST, TEvService)

#undef FILESTORE_FORWARD_REQUEST

}   // namespace NCloud::NFileStore::NStorage
