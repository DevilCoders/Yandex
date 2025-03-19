LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    vhost_server.cpp
)

PEERDIR(
    cloud/blockstore/libs/client
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/endpoints
    cloud/blockstore/libs/service
    cloud/blockstore/libs/vhost
)

END()
