OWNER(
    g:wizard
    g:base
)

PROTO_LIBRARY()

SRCS(
    markup.proto
)

PEERDIR(
    library/cpp/langmask/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
