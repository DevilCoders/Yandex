LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    listener.cpp
    server.cpp
)

PEERDIR(
    cloud/filestore/libs/endpoint
    cloud/filestore/libs/fuse/vhost
    cloud/filestore/libs/service

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
)

END()
