PROTO_LIBRARY()

OWNER(g:antirobot)

PEERDIR(
    library/cpp/getoptpb/proto
)

SRCS(
    args.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
