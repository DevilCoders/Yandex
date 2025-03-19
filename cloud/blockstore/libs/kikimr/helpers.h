#pragma once

#include "public.h"

#include "trace.h"

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/kikimr/helpers.h>

#include <ydb/core/protos/base.pb.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/event.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

inline bool ShouldRepair(const NKikimrProto::EReplyStatus status)
{
    switch (status) {
        case NKikimrProto::NODATA:
        case NKikimrProto::ERROR:
            return true;

        default:
            return false;
    }
}

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_IMPLEMENT_REQUEST(name, ns)                                 \
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
                                                                               \
        BLOCKSTORE_TRACE_SENT(ctx, &ev->TraceId, this, response);              \
        NCloud::Reply(ctx, *ev, std::move(response));                          \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_REQUEST

#define BLOCKSTORE_HANDLE_REQUEST(name, ns)                                    \
    HFunc(ns::TEv##name##Request, Handle##name);                               \
// BLOCKSTORE_HANDLE_REQUEST

#define BLOCKSTORE_REJECT_REQUEST(name, ns)                                    \
    HFunc(ns::TEv##name##Request, Reject##name);                               \
// BLOCKSTORE_REJECT_REQUEST

}   // namespace NCloud::NBlockStore
