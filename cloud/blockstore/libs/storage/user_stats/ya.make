LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    user_stats.cpp
    user_stats_actor.cpp
)

PEERDIR(
    cloud/blockstore/libs/storage/protos

    library/cpp/actors/core

    ydb/core/base
    ydb/core/mon
)

END()
