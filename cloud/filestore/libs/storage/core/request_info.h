#pragma once

#include "public.h"

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/storage/api/events.h>

#include <cloud/storage/core/libs/kikimr/helpers.h>

#include <library/cpp/lwtrace/shuttle.h>

#include <util/generic/intrlist.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/datetime.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TRequestInfo
    : public TAtomicRefCount<TRequestInfo>
    , public TIntrusiveListItem<TRequestInfo>
{
    using TCancelRoutine = void(
        const NActors::TActorContext& ctx,
        TRequestInfo& requestInfo);

    const NActors::TActorId Sender;
    const ui64 Cookie = 0;

    TCallContextPtr CallContext = MakeIntrusive<TCallContext>();
    NWilson::TTraceId TraceId;
    TCancelRoutine* CancelRoutine = nullptr;

    const ui64 Started = GetCycleCount();
    ui64 ExecCycles = 0;

    TRequestInfo() = default;

    TRequestInfo(
            const NActors::TActorId& sender,
            ui64 cookie,
            TCallContextPtr callContext,
            NWilson::TTraceId traceId)
        : Sender(sender)
        , Cookie(cookie)
        , CallContext(std::move(callContext))
        , TraceId(std::move(traceId))
    {}

    void CancelRequest(const NActors::TActorContext& ctx)
    {
        Y_VERIFY(CancelRoutine);
        CancelRoutine(ctx, *this);
    }

    void AddExecCycles(ui64 cycles)
    {
        AtomicAdd(ExecCycles, cycles);
    }

    ui64 GetTotalCycles() const
    {
        return GetCycleCount() - Started;
    }

    ui64 GetExecCycles() const
    {
        return AtomicGet(ExecCycles);
    }

    ui64 GetWaitCycles() const
    {
        return GetTotalCycles() - GetExecCycles();
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TRequestScope
{
    TRequestInfo& RequestInfo;

    const ui64 Started = GetCycleCount();

    TRequestScope(TRequestInfo& requestInfo)
        : RequestInfo(requestInfo)
    {}

    ~TRequestScope()
    {
        Finish();
    }

    ui64 Finish()
    {
        return AtomicAdd(RequestInfo.ExecCycles, GetCycleCount() - Started);
    }
};

////////////////////////////////////////////////////////////////////////////////

inline TRequestInfoPtr CreateRequestInfo(
    const NActors::TActorId& sender,
    ui64 cookie,
    TCallContextPtr callContext,
    NWilson::TTraceId traceId = {})
{
    return MakeIntrusive<TRequestInfo>(
        sender,
        cookie,
        std::move(callContext),
        std::move(traceId));
}

template <typename TMethod>
TRequestInfoPtr CreateRequestInfo(
    const NActors::TActorId& sender,
    ui64 cookie,
    TCallContextPtr callContext,
    NWilson::TTraceId traceId = {})
{
    auto requestInfo = MakeIntrusive<TRequestInfo>(
        sender,
        cookie,
        std::move(callContext),
        std::move(traceId));

    requestInfo->CancelRoutine = [] (
        const NActors::TActorContext& ctx,
        TRequestInfo& requestInfo)
    {
        auto response = std::make_unique<typename TMethod::TResponse>(
            MakeError(E_REJECTED, "tablet is dead"));

        NCloud::Reply(ctx, requestInfo, std::move(response));
    };

    return requestInfo;
}

}   // namespace NCloud::NFileStore::NStorage
