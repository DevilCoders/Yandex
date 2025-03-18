PROTO_LIBRARY()

OWNER(
    g:base
    mvel
)

PEERDIR(
    library/cpp/proto_config/protos
)

SRCS(
    config.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()

