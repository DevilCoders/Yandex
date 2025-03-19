PROTO_LIBRARY()

OWNER(sashateh)

SRCS(
    author.proto
    authority.proto
    common.proto
    content.proto
    dl.proto
    fresh_feeds.proto
    ofeed.proto
    biased.proto
    quotes.proto
    trees.proto
    statistics.proto
)

PEERDIR(
    kernel/blogs/protos
    kernel/indexann/protos
    library/cpp/langmask/proto
)

IF (NOT PY_PROTOS_FOR)
    EXCLUDE_TAGS(GO_PROTO)
ENDIF()

END()
