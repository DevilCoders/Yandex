PROTO_LIBRARY()

OWNER(g:cloud-nbs)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    cloud/filestore/public/api/protos
    cloud/storage/core/protos
)

SRCS(
    tablet.proto
)

END()
