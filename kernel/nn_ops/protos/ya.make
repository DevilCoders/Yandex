PROTO_LIBRARY()

OWNER(olegator)

SRCS(
    delivery_annotation_embeddings.proto
    models_meta.proto
)

PEERDIR(
    mapreduce/yt/interface/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
