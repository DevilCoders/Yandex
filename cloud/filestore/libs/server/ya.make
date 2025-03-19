LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    probes.cpp
    server.cpp
)

PEERDIR(
    cloud/filestore/config
    cloud/filestore/libs/diagnostics
    cloud/filestore/public/api/grpc
    cloud/filestore/public/api/protos

    cloud/filestore/libs/service

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/grpc

    library/cpp/deprecated/atomic
    library/cpp/lwtrace

    contrib/libs/grpc
)

END()

RECURSE_FOR_TESTS(ut)
