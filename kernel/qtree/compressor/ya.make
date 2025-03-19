LIBRARY()

OWNER(
    g:base
    g:wizard
)

PEERDIR(
    library/cpp/blockcodecs/codecs/zstd
    library/cpp/blockcodecs/core
    library/cpp/streams/lz
)

SRCS(
    factory.cpp
)

GENERATE_ENUM_SERIALIZATION(factory.h)

END()
