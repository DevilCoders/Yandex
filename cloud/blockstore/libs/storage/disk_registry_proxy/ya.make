LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    disk_registry_proxy.cpp
)

PEERDIR(
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/core
    cloud/storage/core/libs/api
    library/cpp/actors/core
    ydb/core/base
    ydb/core/mon
    ydb/core/tablet
    ydb/core/tablet_flat
    ydb/core/testlib
    ydb/core/testlib/basics
)

END()

RECURSE_FOR_TESTS(
    ut
)
