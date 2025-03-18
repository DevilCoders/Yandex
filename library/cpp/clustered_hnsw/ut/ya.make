UNITTEST_FOR(library/cpp/clustered_hnsw)

OWNER(
    g:base
    sankear
    nikita-uvarov
)

SRCS(
    clustered_hnsw_ut.cpp
)

PEERDIR(
    library/cpp/clustered_hnsw
    library/cpp/kmeans_hnsw
    library/cpp/hnsw/index
    library/cpp/hnsw/index_builder
    util
)

END()
