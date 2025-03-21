LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    partition_info.cpp
    partition_requests.cpp
    tracing.cpp

    volume.cpp
    volume_actor_addclient.cpp
    volume_actor_allocatedisk.cpp
    volume_actor_checkpoint.cpp
    volume_actor_cleanup_history.cpp
    volume_actor_forward.cpp
    volume_actor_forward_nonrepl.cpp
    volume_actor_forward_trackused.cpp
    volume_actor_initschema.cpp
    volume_actor_loadstate.cpp
    volume_actor_migration.cpp
    volume_actor_monitoring_checkpoint.cpp
    volume_actor_monitoring_removeclient.cpp
    volume_actor_monitoring.cpp
    volume_actor_read_history.cpp
    volume_actor_reallocatedisk.cpp
    volume_actor_removeclient.cpp
    volume_actor_reset_seqnumber.cpp
    volume_actor_startstop.cpp
    volume_actor_stats.cpp
    volume_actor_statvolume.cpp
    volume_actor_throttling.cpp
    volume_actor_updateconfig.cpp
    volume_actor_updateusedblocks.cpp
    volume_actor_usage.cpp
    volume_actor_waitready.cpp
    volume_actor_write_throttlerstate.cpp
    volume_actor.cpp
    volume_counters.cpp
    volume_database.cpp
    volume_state.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/bootstrapper
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/partition
    cloud/blockstore/libs/storage/partition2
    cloud/blockstore/libs/storage/partition_nonrepl
    cloud/blockstore/libs/storage/protos
    cloud/blockstore/libs/storage/volume/model
    library/cpp/actors/core
    library/cpp/lwtrace
    library/cpp/monlib/service/pages
    library/cpp/protobuf/util
    ydb/core/base
    ydb/core/blockstore/core
    ydb/core/mind
    ydb/core/node_whiteboard
    ydb/core/scheme
    ydb/core/tablet
    ydb/core/tablet_flat
)

END()

RECURSE(
    model
)

RECURSE_FOR_TESTS(
    ut
)
