LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    buffer.cpp
    client.cpp
    list.cpp
    poll.cpp
    probes.cpp
    protobuf.cpp
    protocol.cpp
    rcu.cpp
    rdma.cpp
    server.cpp
    utils.cpp
    verbs.cpp
    work_queue.cpp
)

PEERDIR(
    cloud/blockstore/libs/service
    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    library/cpp/monlib/dynamic_counters
    library/cpp/threading/future
    contrib/libs/ibdrv
    contrib/libs/protobuf
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
