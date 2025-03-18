PROTO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

ONLY_TAGS(GO_PROTO)

GRPC()

SRCS(
    common.proto
    origins.proto
    resource.proto
)

END()
