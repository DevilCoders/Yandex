LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    copy_range.cpp
    mirror.cpp

    part_mirror.cpp
    part_mirror_actor.cpp
    part_mirror_actor_mirror.cpp
    part_mirror_actor_readblocks.cpp
    part_mirror_actor_resync.cpp
    part_mirror_actor_stats.cpp
    part_mirror_state.cpp

    part_nonrepl.cpp
    part_nonrepl_actor.cpp
    part_nonrepl_actor_readblocks.cpp
    part_nonrepl_actor_readblocks_local.cpp
    part_nonrepl_actor_stats.cpp
    part_nonrepl_actor_writeblocks.cpp
    part_nonrepl_actor_zeroblocks.cpp

    part_nonrepl_rdma.cpp
    part_nonrepl_rdma_actor.cpp
    part_nonrepl_rdma_actor_readblocks.cpp
    part_nonrepl_rdma_actor_readblocks_local.cpp
    part_nonrepl_rdma_actor_stats.cpp
    part_nonrepl_rdma_actor_writeblocks.cpp
    part_nonrepl_rdma_actor_zeroblocks.cpp

    part_nonrepl_migration.cpp
    part_nonrepl_migration_actor.cpp
    part_nonrepl_migration_actor_migration.cpp
    part_nonrepl_migration_actor_mirror.cpp
    part_nonrepl_migration_actor_readblocks.cpp
    part_nonrepl_migration_actor_readblocks_local.cpp
    part_nonrepl_migration_actor_stats.cpp
    part_nonrepl_migration_state.cpp

    part_nonrepl_util.cpp
)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/libs/common
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/rdma
    cloud/blockstore/libs/service_local
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/protos
    library/cpp/actors/core
    ydb/core/base
    ydb/core/testlib
    ydb/core/testlib/basics
)

END()

RECURSE_FOR_TESTS(
    ut
)
