LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    spdk_client.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/service
    cloud/blockstore/libs/spdk

    cloud/storage/core/libs/diagnostics
)

END()
