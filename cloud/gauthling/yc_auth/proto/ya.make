OWNER(g:cloud)

PROTO_LIBRARY()

GRPC()

SRCS(
    scms_agent.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
