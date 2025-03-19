PROTO_LIBRARY()

OWNER(
    filmih
    g:neural-search
)

SRCS(
    model_embedding.proto
)

PEERDIR()

GENERATE_ENUM_SERIALIZATION(
    model_embedding.pb.h
)

EXCLUDE_TAGS(GO_PROTO)

END()
