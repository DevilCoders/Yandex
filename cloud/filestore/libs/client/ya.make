LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    client.cpp
    config.cpp
    durable.cpp
    probes.cpp
    session.cpp
)

PEERDIR(
    cloud/filestore/config
    cloud/filestore/public/api/grpc
    cloud/filestore/public/api/protos

    cloud/filestore/libs/service

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/grpc

    library/cpp/lwtrace

    contrib/libs/grpc
)

END()

RECURSE_FOR_TESTS(ut)
