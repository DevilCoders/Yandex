OWNER(
    g:ugc
    stakanviski
)

PROTO_LIBRARY()

SRCS(
    test_schema.proto
)

PEERDIR(
    kernel/ugc/proto
    kernel/ugc/schema/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
