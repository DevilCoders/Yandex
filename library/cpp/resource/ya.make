LIBRARY()

OWNER(pg)

PEERDIR(
    library/cpp/blockcodecs/core
    library/cpp/blockcodecs/codecs/zstd
)

SRCS(
    registry.cpp
    resource.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
