LIBRARY()

OWNER(g:cloud-nbs)

GENERATE_ENUM_SERIALIZATION(options.h)

SRCS(
    bootstrap.cpp
    options.cpp
)

PEERDIR(
    cloud/filestore/config
    cloud/filestore/libs/diagnostics
    cloud/filestore/libs/server
    cloud/filestore/libs/storage/core
    cloud/filestore/libs/storage/init

    cloud/filestore/libs/service

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/version

    library/cpp/lwtrace

    ydb/core/protos
)

END()