PROTO_LIBRARY()

OWNER(
    g:ydo
)

PEERDIR(
    ydo/libs/geobuf/proto
)

SRCS(
    embeddings.proto
    geometry.proto
    messages.proto
    reviews.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
