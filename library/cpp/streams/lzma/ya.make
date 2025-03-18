LIBRARY()

OWNER(
    pg
    g:util
)

PEERDIR(
    contrib/libs/lzmasdk
)

SRCS(
    lzma.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
