#pragma once

#include "public.h"

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>

#include <library/cpp/actors/core/actorid.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_DISK_REGISTRY_PROXY_REQUESTS(xxx, ...)                      \
    xxx(Subscribe,            __VA_ARGS__)                                     \
    xxx(Unsubscribe,          __VA_ARGS__)                                     \
    xxx(Reassign,             __VA_ARGS__)                                     \
// BLOCKSTORE_DISK_REGISTRY_PROXY_REQUESTS

////////////////////////////////////////////////////////////////////////////////

struct TEvDiskRegistryProxy
{
    //
    // Subscribe
    //

    struct TSubscribeRequest
    {
        NActors::TActorId Subscriber;

        explicit TSubscribeRequest(NActors::TActorId subscriber)
            : Subscriber(std::move(subscriber))
        {}
    };

    struct TSubscribeResponse
    {
        const bool Connected = false;

        TSubscribeResponse() = default;

        explicit TSubscribeResponse(bool connected)
            : Connected(connected)
        {}
    };

    //
    // Unsubscribe
    //

    struct TUnsubscribeRequest
    {
        NActors::TActorId Subscriber;

        explicit TUnsubscribeRequest(NActors::TActorId subscriber)
            : Subscriber(std::move(subscriber))
        {}
    };

    struct TUnsubscribeResponse
    {
    };

    //
    // ConnectionLost notification
    //

    struct TConnectionLost
    {
    };

    //
    // Reassign
    //

    struct TReassignRequest
    {
        TString SysKind;
        TString LogKind;
        TString IndexKind;

        TReassignRequest(
                TString sysKind,
                TString logKind,
                TString indexKind)
            : SysKind(std::move(sysKind))
            , LogKind(std::move(logKind))
            , IndexKind(std::move(indexKind))
        {}
    };

    struct TReassignResponse
    {
    };

    //
    // DiskRegistryCreateResult notification
    //

    struct TDiskRegistryCreateResult
    {
        const ui64 TabletId = 0;

        TDiskRegistryCreateResult() = default;

        explicit TDiskRegistryCreateResult(ui64 tabletId)
            : TabletId(tabletId)
        {}
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStoreEvents::DISK_REGISTRY_PROXY_START,

        EvSubscribeRequest = EvBegin + 1,
        EvSubscribeResponse = EvBegin + 2,

        EvUnsubscribeRequest = EvBegin + 3,
        EvUnsubscribeResponse = EvBegin + 4,

        EvConnectionLost = EvBegin + 5,

        EvReassignRequest = EvBegin + 6,
        EvReassignResponse = EvBegin + 7,

        EvDiskRegistryCreateResult = EvBegin + 8,

        EvEnd
    };

    static_assert(EvEnd < (int)TBlockStoreEvents::DISK_REGISTRY_PROXY_END,
        "EvEnd expected to be < TBlockStoreEvents::DISK_REGISTRY_PROXY_END");

    BLOCKSTORE_DISK_REGISTRY_PROXY_REQUESTS(BLOCKSTORE_DECLARE_EVENTS)

    using TEvConnectionLost = TResponseEvent<TConnectionLost, EvConnectionLost>;
    using TEvDiskRegistryCreateResult =
        TResponseEvent<TDiskRegistryCreateResult, EvDiskRegistryCreateResult>;
};

////////////////////////////////////////////////////////////////////////////////

NActors::TActorId MakeDiskRegistryProxyServiceId();

}   // namespace NCloud::NBlockStore::NStorage
