LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    actor_loadfreshblobs.cpp
    actor_trimfreshlog.cpp
)

PEERDIR(
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/partition_common/model
    cloud/storage/core/libs/common
    cloud/storage/core/libs/kikimr
    library/cpp/actors/core
    ydb/core/base
)

END()

RECURSE(
    model
)
