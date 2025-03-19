PROTO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    loadtest.proto
)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/public/api/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
