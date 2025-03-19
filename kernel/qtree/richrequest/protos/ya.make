OWNER(
    g:wizard
    g:base
)

PROTO_LIBRARY()

SRCS(
    rich_tree.proto
    lemmer_cache_key.proto
    thesaurus_exttype.proto
    proxim.proto
)

PEERDIR(
    kernel/qtree/richrequest/markup/protos
    library/cpp/langmask/proto
    library/cpp/token/serialization/protos
)

GENERATE_ENUM_SERIALIZATION(thesaurus_exttype.pb.h)

EXCLUDE_TAGS(GO_PROTO)

END()
