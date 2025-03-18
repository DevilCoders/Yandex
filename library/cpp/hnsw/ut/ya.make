UNITTEST()

OWNER(alipov)

SRCS(
    main.cpp
    mobius_transform.cpp
)

PEERDIR(
    library/cpp/hnsw/index
    library/cpp/hnsw/index_builder
    util
)

ALLOCATOR(LF)

END()
