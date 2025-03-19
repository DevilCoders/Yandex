#include "helpers.h"

#include <library/cpp/actors/core/log.h>

namespace NCloud {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

TString EventInfo(TAutoPtr<IEventHandle>& ev)
{
    if (ev->HasEvent()) {
        const auto* event = ev->GetBase();
        return event->ToStringHeader();
    }

    if (ev->HasBuffer()) {
        return "<not loaded>";
    }

    return "<null>";
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

NProto::TError MakeKikimrError(NKikimrProto::EReplyStatus status, TString errorReason)
{
    NProto::TError error;

    if (status != NKikimrProto::OK) {
        error.SetCode(MAKE_KIKIMR_ERROR(status));

        if (errorReason) {
            error.SetMessage(std::move(errorReason));
        } else {
            error.SetMessage(NKikimrProto::EReplyStatus_Name(status));
        }
    }

    return error;
}

NProto::TError MakeSchemeShardError(NKikimrScheme::EStatus status, TString errorReason)
{
    NProto::TError error;

    if (status != NKikimrScheme::StatusSuccess) {
        error.SetCode(MAKE_SCHEMESHARD_ERROR(status));

        if (errorReason) {
            error.SetMessage(std::move(errorReason));
        } else {
            error.SetMessage(NKikimrScheme::EStatus_Name(status));
        }
    }

    return error;
}

void HandleUnexpectedEvent(
    const TActorContext& ctx,
    TAutoPtr<IEventHandle>& ev,
    int component)
{
#if defined(NDEBUG)
    LogUnexpectedEvent(ctx, ev, component);
#else
    Y_UNUSED(ctx);
    Y_FAIL(
        "[%s] Unexpected event: (0x%08X) %s",
        ctx.LoggerSettings()->ComponentName(component),
        ev->GetTypeRewrite(),
        EventInfo(ev).c_str());
#endif
}

void LogUnexpectedEvent(
    const TActorContext& ctx,
    TAutoPtr<IEventHandle>& ev,
    int component)
{
    LOG_ERROR(ctx, component,
        "Unexpected event: (0x%08X) %s",
        ev->GetTypeRewrite(),
        EventInfo(ev).c_str());
}

}   // namespace NCloud
