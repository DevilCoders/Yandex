PROTO_LIBRARY()

OWNER(g:base)

SRCS(
    attr_restriction_iterator.proto
    attr_restriction_iterators.proto
)

PEERDIR(
    kernel/attributes/restrictions/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
