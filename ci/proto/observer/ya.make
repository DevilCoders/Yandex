OWNER(g:ci)

PROTO_LIBRARY()

GRPC()

PEERDIR(
    ci/proto/storage
)

SRCS(
    internal.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
