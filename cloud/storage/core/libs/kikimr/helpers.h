#pragma once

#include "public.h"

#include <cloud/storage/core/libs/common/error.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/event.h>

#include <ydb/core/protos/base.pb.h>
#include <ydb/core/protos/flat_tx_scheme.pb.h>

#include <util/generic/string.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

NProto::TError MakeKikimrError(NKikimrProto::EReplyStatus status, TString errorReason = {});
NProto::TError MakeSchemeShardError(NKikimrScheme::EStatus status, TString errorReason = {});

void HandleUnexpectedEvent(
    const NActors::TActorContext& ctx,
    TAutoPtr<NActors::IEventHandle>& ev,
    int component);

void LogUnexpectedEvent(
    const NActors::TActorContext& ctx,
    TAutoPtr<NActors::IEventHandle>& ev,
    int component);

inline NActors::TActorId Register(
    const NActors::TActorContext& ctx,
    NActors::IActorPtr actor)
{
    return ctx.Register(actor.release());
}

inline NActors::TActorId RegisterLocal(
    const NActors::TActorContext& ctx,
    NActors::IActorPtr actor)
{
    return ctx.RegisterWithSameMailbox(actor.release());
}

template <typename T, typename ...TArgs>
inline NActors::TActorId Register(
    const NActors::TActorContext& ctx,
    TArgs&& ...args)
{
    auto actor = std::make_unique<T>(std::forward<TArgs>(args)...);
    return Register(ctx, std::move(actor));
}

template <typename T, typename ...TArgs>
inline NActors::TActorId RegisterLocal(
    const NActors::TActorContext& ctx,
    TArgs&& ...args)
{
    auto actor = std::make_unique<T>(std::forward<TArgs>(args)...);
    return RegisterLocal(ctx, std::move(actor));
}

inline void Send(
    const NActors::TActorContext& ctx,
    const NActors::TActorId& recipient,
    NActors::IEventBasePtr event,
    ui64 cookie = 0,
    NWilson::TTraceId traceId = {})
{
    ctx.Send(
        recipient,
        event.release(),
        0,  // flags
        cookie,
        std::move(traceId));
}

inline void SendWithUndeliveryTracking(
    const NActors::TActorContext& ctx,
    const NActors::TActorId& recipient,
    NActors::IEventBasePtr event,
    ui64 cookie = 0,
    NWilson::TTraceId traceId = {})
{
    auto ev = std::make_unique<NActors::IEventHandle>(
        recipient,
        ctx.SelfID,
        event.release(),
        NActors::IEventHandle::FlagForwardOnNondelivery,    // flags
        cookie,  // cookie
        &ctx.SelfID,    // forwardOnNondelivery
        traceId.Clone());

    ctx.Send(ev.release());
}

template <typename T>
inline void Send(
    const NActors::TActorContext& ctx,
    const NActors::TActorId& recipient,
    ui64 cookie = 0,
    NWilson::TTraceId traceId = {})
{
    Send(ctx, recipient, std::make_unique<T>(), cookie, std::move(traceId));
}

template <typename T, typename ...TArgs>
inline void Send(
    const NActors::TActorContext& ctx,
    const NActors::TActorId& recipient,
    ui64 cookie,
    TArgs&& ...args)
{
    auto event = std::make_unique<T>(std::forward<TArgs>(args)...);
    Send(ctx, recipient, std::move(event), cookie);
}

template <typename T>
inline void Reply(
    const NActors::TActorContext& ctx,
    T& request,
    NActors::IEventBasePtr response)
{
    ctx.Send(
        request.Sender,
        response.release(),
        0,  // flags
        request.Cookie,
        std::move(request.TraceId));
}

template <typename T>
inline void ReplyNoTrace(
    const NActors::TActorContext& ctx,
    T& request,
    NActors::IEventBasePtr response)
{
    ctx.Send(
        request.Sender,
        response.release(),
        0,  // flags
        request.Cookie,
        {});
}

////////////////////////////////////////////////////////////////////////////////

#define STORAGE_IMPLEMENT_REQUEST(name, ns)                                    \
    void Handle##name(                                                         \
        const ns::TEv##name##Request::TPtr& ev,                                \
        const NActors::TActorContext& ctx);                                    \
                                                                               \
    void Reject##name(                                                         \
        const ns::TEv##name##Request::TPtr& ev,                                \
        const NActors::TActorContext& ctx)                                     \
    {                                                                          \
        auto response = std::make_unique<ns::TEv##name##Response>(             \
            MakeError(E_REJECTED, #name " request rejected"));                 \
        NCloud::Reply(ctx, *ev, std::move(response));                          \
    }                                                                          \
// STORAGE_IMPLEMENT_REQUEST

#define STORAGE_HANDLE_REQUEST(name, ns)                                       \
    HFunc(ns::TEv##name##Request, Handle##name);                               \
// STORAGE_HANDLE_REQUEST

#define STORAGE_REJECT_REQUEST(name, ns)                                       \
    HFunc(ns::TEv##name##Request, Reject##name);                               \
// STORAGE_REJECT_REQUEST

}   // namespace NCloud
