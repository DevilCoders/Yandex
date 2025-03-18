LIBRARY()

OWNER(
    bulatman
    g:util
)

PEERDIR(
    contrib/libs/zstd
)

SRCS(
    zstd.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
