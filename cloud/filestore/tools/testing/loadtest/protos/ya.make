PROTO_LIBRARY()

OWNER(g:cloud-nbs)

EXCLUDE_TAGS(GO_PROTO PY_PROTO JAVA_PROTO)

PEERDIR(
    cloud/filestore/public/api/protos
)

SRCS(
    loadtest.proto
)

END()
