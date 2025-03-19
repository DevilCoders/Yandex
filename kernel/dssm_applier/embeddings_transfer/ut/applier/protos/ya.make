PROTO_LIBRARY()

OWNER(
    filmih
    g:neural-search
    e-shalnov
)

NEED_CHECK()

SRCS(
    config.proto
)

PEERDIR(
    library/cpp/getoptpb/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
