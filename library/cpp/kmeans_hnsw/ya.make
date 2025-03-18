LIBRARY()

OWNER(
    g:base
    sankear
    nikita-uvarov
)

SRCS(
    all.cpp
)

PEERDIR(
    library/cpp/hnsw/index_builder
    library/cpp/hnsw/index
    library/cpp/dot_product
    library/cpp/threading/local_executor
    util
)

END()
