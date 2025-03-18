PROTO_LIBRARY()

OWNER(g:go-library)

INCLUDE_TAGS(GO_PROTO)

GRPC()

SRCS(
    test.proto
)

END()
