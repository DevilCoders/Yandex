PROTO_LIBRARY()

OWNER (g:cs_dev)

PEERDIR(
    library/cpp/getoptpb/proto
)

SRCS(
    command.proto
)

EXCLUDE_TAGS(GO_PROTO JAVA_PROTO)

END()
