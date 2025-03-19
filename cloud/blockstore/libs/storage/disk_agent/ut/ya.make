UNITTEST_FOR(cloud/blockstore/libs/storage/disk_agent)

OWNER(g:cloud-nbs)

SRCS(
    disk_agent_state_ut.cpp
    disk_agent_ut.cpp
    spdk_initializer_ut.cpp
    storage_with_stats_ut.cpp
)

PEERDIR(
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/disk_agent/testlib
    library/cpp/actors/core
    library/cpp/testing/unittest
    ydb/core/testlib
    ydb/core/testlib/basics
)

REQUIREMENTS(ram_disk:1)

END()
