UNITTEST_FOR(library/cpp/kmeans_hnsw)

OWNER(
    g:base
    sankear
    nikita-uvarov
)

SRCS(
    kmeans_clustering_ut.cpp
)

PEERDIR(
    library/cpp/kmeans_hnsw
    library/cpp/dot_product
    util
)

REQUIREMENTS(network:full)

END()
