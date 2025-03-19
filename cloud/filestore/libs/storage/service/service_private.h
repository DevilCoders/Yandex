#pragma once

#include "public.h"

#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/storage/api/components.h>
#include <cloud/filestore/libs/storage/api/events.h>
#include <cloud/filestore/libs/storage/core/public.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_SERVICE_REQUESTS_PRIVATE(xxx, ...)                           \
// FILESTORE_SERVICE_REQUESTS_PRIVATE

////////////////////////////////////////////////////////////////////////////////

struct TEvServicePrivate
{
    //
    // Session start/stop
    //

    struct TSessionCreated
    {
        TString ClientId;
        TString SessionId;
        TString SessionState;
        ui64 TabletId = 0;
        NProto::TFileStore FileStore;
        TRequestInfoPtr RequestInfo;
    };

    struct TSessionDestroyed
    {
        TString SessionId;
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TFileStoreEventsPrivate::TABLET_START,

        FILESTORE_SERVICE_REQUESTS_PRIVATE(FILESTORE_DECLARE_EVENT_IDS)

        EvPingSession,
        EvSessionCreated,
        EvSessionDestroyed,
        EvUpdateStats,

        EvEnd
    };

    static_assert(EvEnd < (int)TFileStoreEventsPrivate::TABLET_END,
        "EvEnd expected to be < TFileStoreEventsPrivate::TABLET_END");

    FILESTORE_SERVICE_REQUESTS_PRIVATE(FILESTORE_DECLARE_EVENTS)

    using TEvPingSession = TRequestEvent<TEmpty, EvPingSession>;
    using TEvSessionCreated = TResponseEvent<TSessionCreated, EvSessionCreated>;
    using TEvSessionDestroyed = TResponseEvent<TSessionDestroyed, EvSessionDestroyed>;
    using TEvUpdateStats = TRequestEvent<TEmpty, EvUpdateStats>;
};

}   // namespace NCloud::NFileStore::NStorage
