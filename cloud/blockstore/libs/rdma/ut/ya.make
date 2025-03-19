UNITTEST_FOR(cloud/blockstore/libs/rdma)

OWNER(g:cloud-nbs)

SRCS(
    buffer_ut.cpp
    list_ut.cpp
    poll_ut.cpp
    protobuf_ut.cpp
)

PEERDIR(
    cloud/blockstore/public/api/protos
)

END()
