PROTO_LIBRARY()

OWNER(g:antirobot)

PEERDIR(
    library/cpp/getoptpb/proto
)

SRCS(
    config.proto
    cbb_response.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
