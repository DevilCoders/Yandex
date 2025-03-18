PROTO_LIBRARY()

OWNER(g:aapi)

GRPC()

SRCS(
    types.proto
    vcs.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
