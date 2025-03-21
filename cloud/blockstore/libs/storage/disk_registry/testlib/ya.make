LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    test_env.cpp
    test_logbroker.cpp
    test_state.cpp
)

PEERDIR(
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/disk_registry
    library/cpp/actors/core
    library/cpp/testing/unittest
    ydb/core/testlib
    ydb/core/testlib/basics
)

END()
