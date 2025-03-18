UNITTEST()

OWNER(
    smirnovpavel
    g:alice_quality
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/hnsw/index
    library/cpp/online_hnsw/base
    library/cpp/testing/unittest
    util
)

END()
