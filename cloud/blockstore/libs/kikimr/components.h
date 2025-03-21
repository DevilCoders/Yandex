#pragma once

#include "public.h"

#include <ydb/core/base/events.h>
#include <ydb/core/protos/services.pb.h>

#include <library/cpp/actors/core/events.h>

#include <util/generic/string.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_ACTORS(xxx)                                                 \
    xxx(BOOTSTRAPPER)                                                          \
    xxx(SERVICE)                                                               \
    xxx(SERVICE_PROXY)                                                         \
    xxx(VOLUME)                                                                \
    xxx(PARTITION)                                                             \
    xxx(PARTITION_WORKER)                                                      \
    xxx(SCHEDULER)                                                             \
    xxx(HIVE_PROXY)                                                            \
    xxx(SS_PROXY)                                                              \
    xxx(METERING)                                                              \
    xxx(SCHEMESHARD)                                                           \
    xxx(VOLUME_PROXY)                                                          \
    xxx(AUTH)                                                                  \
    xxx(DISK_AGENT)                                                            \
    xxx(DISK_AGENT_WORKER)                                                     \
    xxx(DISK_REGISTRY)                                                         \
    xxx(DISK_REGISTRY_WORKER)                                                  \
    xxx(DISK_REGISTRY_PROXY)                                                   \
    xxx(VOLUME_BALANCER)                                                       \
    xxx(PARTITION_NONREPL)                                                     \
    xxx(PARTITION_COMMON)                                                      \
    xxx(STATS_SERVICE)                                                         \
    xxx(USER_STATS)                                                            \
// BLOCKSTORE_ACTORS

#define BLOCKSTORE_COMPONENTS(xxx)                                             \
    xxx(SERVER)                                                                \
    xxx(TRACE)                                                                 \
    xxx(DISCOVERY)                                                             \
    xxx(YDBSTATS)                                                              \
    xxx(CLIENT)                                                                \
    xxx(NBD)                                                                   \
    xxx(VHOST)                                                                 \
    xxx(SPDK)                                                                  \
    xxx(LOGBROKER)                                                             \
    xxx(NOTIFY)                                                                \
    xxx(RDMA)                                                                  \
    xxx(LOCAL_STORAGE)                                                         \
    BLOCKSTORE_ACTORS(xxx)                                                     \
// BLOCKSTORE_COMPONENTS

////////////////////////////////////////////////////////////////////////////////

struct TBlockStoreComponents
{
    enum
    {
        START = 1024,   // TODO

#define BLOCKSTORE_DECLARE_COMPONENT(component)                                \
        component,                                                             \
// BLOCKSTORE_DECLARE_COMPONENT

        BLOCKSTORE_COMPONENTS(BLOCKSTORE_DECLARE_COMPONENT)

#undef BLOCKSTORE_DECLARE_COMPONENT

        END
    };
};

const TString& GetComponentName(int component);

////////////////////////////////////////////////////////////////////////////////

struct TBlockStoreEvents
{
    enum
    {
        START = EventSpaceBegin(NKikimr::TKikimrEvents::ES_BLOCKSTORE),

#define BLOCKSTORE_DECLARE_COMPONENT(component)                                \
        component##_START,                                                     \
        component##_END = component##_START + 100,                             \
// BLOCKSTORE_DECLARE_COMPONENT

        BLOCKSTORE_ACTORS(BLOCKSTORE_DECLARE_COMPONENT)

#undef BLOCKSTORE_DECLARE_COMPONENT

        END
    };

    static_assert(END < EventSpaceEnd(NKikimr::TKikimrEvents::ES_BLOCKSTORE),
        "END expected to be < EventSpaceEnd(NKikimr::TKikimrEvents::BLOCKSTORE)");

    // reserved for SchemeShard integration
    static_assert(SCHEMESHARD_START == START + 1011,
        "SCHEMESHARD_START expected to be == START + 1011");
};

////////////////////////////////////////////////////////////////////////////////

struct TBlockStorePrivateEvents
{
    enum
    {
        START = EventSpaceBegin(NKikimr::TKikimrEvents::ES_BLOCKSTORE_PRIVATE),

#define BLOCKSTORE_DECLARE_COMPONENT(component)                                \
        component##_START,                                                     \
        component##_END = component##_START + 100,                             \
// BLOCKSTORE_DECLARE_COMPONENT

        BLOCKSTORE_ACTORS(BLOCKSTORE_DECLARE_COMPONENT)

#undef BLOCKSTORE_DECLARE_COMPONENT

        END
    };

    static_assert(END < EventSpaceEnd(NKikimr::TKikimrEvents::ES_BLOCKSTORE_PRIVATE),
        "END expected to be < EventSpaceEnd(NKikimr::TKikimrEvents::BLOCKSTORE)");
};

////////////////////////////////////////////////////////////////////////////////

struct TBlockStoreActivities
{
    enum
    {
#define BLOCKSTORE_DECLARE_COMPONENT(component)                                \
        component = NKikimrServices::TActivity::BLOCKSTORE_##component,        \
// BLOCKSTORE_DECLARE_COMPONENT

        BLOCKSTORE_ACTORS(BLOCKSTORE_DECLARE_COMPONENT)

#undef BLOCKSTORE_DECLARE_COMPONENT
    };
};

}   // namespace NCloud::NBlockStore
