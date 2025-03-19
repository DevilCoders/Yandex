LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    client.cpp
    config.cpp
    durable.cpp
    metric.cpp
    session.cpp
    session_test.cpp
    throttling.cpp
)

PEERDIR(
    cloud/blockstore/public/api/grpc
    cloud/blockstore/public/api/protos

    cloud/blockstore/config
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/service
    cloud/blockstore/libs/throttling
    cloud/storage/core/libs/grpc

    library/cpp/lwtrace
    library/cpp/monlib/dynamic_counters
    library/cpp/threading/future
    library/cpp/monlib/service
    library/cpp/monlib/service/pages

    contrib/libs/grpc
)

END()

RECURSE_FOR_TESTS(ut)
