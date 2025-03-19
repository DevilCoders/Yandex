PROTO_LIBRARY()

OWNER(g:cloud-nbs)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    cloud/storage/core/protos
)

SRCS(
    checkpoint.proto
    cluster.proto
    const.proto
    data.proto
    endpoint.proto
    fs.proto
    headers.proto
    locks.proto
    node.proto
    ping.proto
    session.proto
)

END()
