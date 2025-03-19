PROTO_LIBRARY()

OWNER(g:cloud-nbs)

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    tablet.proto
)

PEERDIR(
    cloud/filestore/public/api/protos
)

END()
