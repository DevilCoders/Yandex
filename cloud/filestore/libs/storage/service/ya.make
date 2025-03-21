LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    helpers.cpp
    service.cpp
    service_actor.cpp
    service_actor_alterfs.cpp
    service_actor_createfs.cpp
    service_actor_createsession.cpp
    service_actor_describefsmodel.cpp
    service_actor_destroyfs.cpp
    service_actor_destroysession.cpp
    service_actor_forward.cpp
    service_actor_getfsinfo.cpp
    service_actor_getsessionevents.cpp
    service_actor_list.cpp
    service_actor_monitoring.cpp
    service_actor_ping.cpp
    service_actor_pingsession.cpp
    service_actor_statfs.cpp
    service_actor_update_stats.cpp
    service_state.cpp
)

PEERDIR(
    cloud/filestore/libs/storage/api
    cloud/filestore/libs/storage/core
    cloud/filestore/libs/storage/model
    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    library/cpp/actors/core
    library/cpp/monlib/service/pages
    library/cpp/string_utils/quote
    ydb/core/base
    ydb/core/mind
    ydb/core/mon
    ydb/core/protos
    ydb/core/tablet
)

END()

RECURSE_FOR_TESTS(
    ut
)
