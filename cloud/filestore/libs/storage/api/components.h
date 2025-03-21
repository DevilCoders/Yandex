#pragma once

#include "public.h"

#include <ydb/core/base/events.h>
#include <ydb/core/protos/services.pb.h>

#include <library/cpp/actors/core/events.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_ACTORS(xxx)                                                  \
    xxx(SCHEMESHARD)                                                           \
    xxx(SERVICE)                                                               \
    xxx(SERVICE_WORKER)                                                        \
    xxx(SERVICE_PROXY)                                                         \
    xxx(TABLET)                                                                \
    xxx(TABLET_WORKER)                                                         \
    xxx(TABLET_PROXY)                                                          \
    xxx(SS_PROXY)                                                              \
// FILESTORE_ACTORS

#define FILESTORE_COMPONENTS(xxx)                                              \
    xxx(SERVER)                                                                \
    xxx(TRACE)                                                                 \
    xxx(HIVE_PROXY)                                                            \
    FILESTORE_ACTORS(xxx)                                                      \
// FILESTORE_COMPONENTS

////////////////////////////////////////////////////////////////////////////////

struct TFileStoreComponents
{
    enum
    {
        START = 2048,   // TODO

#define FILESTORE_DECLARE_COMPONENT(component)                                 \
        component,                                                             \
// FILESTORE_DECLARE_COMPONENT

        FILESTORE_COMPONENTS(FILESTORE_DECLARE_COMPONENT)

#undef FILESTORE_DECLARE_COMPONENT

        END
    };
};

const TString& GetComponentName(int component);

////////////////////////////////////////////////////////////////////////////////

struct TFileStoreActivities
{
    enum
    {
#define FILESTORE_DECLARE_COMPONENT(component)                                 \
        component = NKikimrServices::TActivity::FILESTORE_##component,         \
// FILESTORE_DECLARE_COMPONENT

        FILESTORE_ACTORS(FILESTORE_DECLARE_COMPONENT)

#undef FILESTORE_DECLARE_COMPONENT
    };
};

////////////////////////////////////////////////////////////////////////////////

struct TFileStoreEvents
{
    enum
    {
        START = EventSpaceBegin(NKikimr::TKikimrEvents::ES_FILESTORE),

#define FILESTORE_DECLARE_COMPONENT(component)                                 \
        component##_START,                                                     \
        component##_END = component##_START + 100,                             \
// FILESTORE_DECLARE_COMPONENT

        FILESTORE_ACTORS(FILESTORE_DECLARE_COMPONENT)

#undef FILESTORE_DECLARE_COMPONENT

        END
    };

    static_assert(END < EventSpaceEnd(NKikimr::TKikimrEvents::ES_FILESTORE),
        "END expected to be < EventSpaceEnd(NKikimr::TKikimrEvents::ES_FILESTORE)");
};

////////////////////////////////////////////////////////////////////////////////

struct TFileStoreEventsPrivate
{
    enum
    {
        START = EventSpaceBegin(NKikimr::TKikimrEvents::ES_FILESTORE_PRIVATE),

#define FILESTORE_DECLARE_COMPONENT(component)                                 \
        component##_START,                                                     \
        component##_END = component##_START + 100,                             \
// FILESTORE_DECLARE_COMPONENT

        FILESTORE_ACTORS(FILESTORE_DECLARE_COMPONENT)

#undef FILESTORE_DECLARE_COMPONENT

        END
    };

    static_assert(END < EventSpaceEnd(NKikimr::TKikimrEvents::ES_FILESTORE_PRIVATE),
        "END expected to be < EventSpaceEnd(NKikimr::TKikimrEvents::ES_FILESTORE_PRIVATE)");
};

}   // namespace NCloud::NFileStore::NStorage
