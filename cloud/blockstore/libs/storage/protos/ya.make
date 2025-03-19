PROTO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    disk.proto
    part.proto
    volume.proto
)

PEERDIR(
    cloud/blockstore/public/api/protos
    cloud/storage/core/protos
    ydb/core/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
