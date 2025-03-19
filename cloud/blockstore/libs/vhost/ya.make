LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    server.cpp
    vhost.cpp
    vhost_test.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/service
    cloud/blockstore/libs/rdma

    cloud/contrib/vhost
)

END()

RECURSE_FOR_TESTS(ut)
