LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    rdma_server.cpp
)

PEERDIR(
    cloud/blockstore/libs/client
    cloud/blockstore/libs/client_rdma
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/endpoints
    cloud/blockstore/libs/rdma
    cloud/blockstore/libs/service

    cloud/storage/core/libs/coroutine
)

END()
