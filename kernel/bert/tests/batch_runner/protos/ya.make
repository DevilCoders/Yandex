PROTO_LIBRARY()

OWNER(
    g:neural-search
    e-shalnov
    lexeyo
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
