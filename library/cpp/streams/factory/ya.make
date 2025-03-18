LIBRARY()

OWNER(
    pg
    g:util
)

PEERDIR(
    library/cpp/streams/bzip2
    library/cpp/streams/lz
)

SRCS(
    factory.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
