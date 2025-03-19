PROTO_LIBRARY()

OWNER(
    g:search-pers
)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    clustered_embedding.proto
    fading_embedding.proto
    embedding_types.proto
    user_history.proto
)

PEERDIR(
    kernel/embeddings_info/proto
)

GENERATE_ENUM_SERIALIZATION(
    user_history.pb.h
)

END()
