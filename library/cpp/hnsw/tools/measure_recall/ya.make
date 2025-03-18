PROGRAM()

OWNER(
    nzinov
    alipov
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/hnsw/index
    library/cpp/hnsw/index_builder
    library/cpp/getopt/small
    util
)

GENERATE_ENUM_SERIALIZATION(option_enums.h)

ALLOCATOR(LF)

END()
