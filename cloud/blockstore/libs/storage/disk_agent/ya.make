LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    disk_agent_actor_acquire.cpp
    disk_agent_actor_init.cpp
    disk_agent_actor_io.cpp
    disk_agent_actor_monitoring.cpp
    disk_agent_actor_register.cpp
    disk_agent_actor_release.cpp
    disk_agent_actor_secure_erase.cpp
    disk_agent_actor_stats.cpp
    disk_agent_actor_waitready.cpp
    disk_agent_actor.cpp
    disk_agent_counters.cpp
    disk_agent_state.cpp
    disk_agent.cpp
    hash_table_storage.cpp
    probes.cpp
    rdma_target.cpp
    spdk_initializer.cpp
    storage_initializer.cpp
    storage_with_stats.cpp
)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/rdma
    cloud/blockstore/libs/service_local
    cloud/blockstore/libs/spdk
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/disk_common
    library/cpp/actors/core
    library/cpp/containers/stack_vector
    ydb/core/base
    ydb/core/mind
    ydb/core/mon
    ydb/core/tablet
    library/cpp/deprecated/atomic
)

ADDINCL(
    contrib/libs/spdk/include
)

END()

RECURSE_FOR_TESTS(
    ut
)

RECURSE_FOR_TESTS(
    ut_large
)
