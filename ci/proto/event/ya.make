OWNER(g:ci)

PROTO_LIBRARY()

GRPC()

SRCS(
    event.proto
    ci.proto
)

PEERDIR(
    ci/tasklet/common/proto
    ci/proto
)


EXCLUDE_TAGS(GO_PROTO)

END()
