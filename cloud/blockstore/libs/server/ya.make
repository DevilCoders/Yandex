LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    client_acceptor.cpp
    config.cpp
    endpoint_poller.cpp
    server.cpp
    server_test.cpp
    socket_poller.cpp
)

PEERDIR(
    cloud/blockstore/public/api/grpc
    cloud/blockstore/public/api/protos

    cloud/blockstore/config
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/service
    cloud/storage/core/libs/grpc

    library/cpp/actors/prof
    library/cpp/monlib/service
    library/cpp/monlib/service/pages

    contrib/libs/grpc
)

IF (PROFILE_MEMORY_ALLOCATIONS)
    CFLAGS(-DPROFILE_MEMORY_ALLOCATIONS)
ENDIF()

END()

RECURSE_FOR_TESTS(ut)
