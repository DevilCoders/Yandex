OWNER(g:ci)

PROTO_LIBRARY()

GRPC()

SRCS(
    graph_discovery_admin.proto
)

PEERDIR(
    ci/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
