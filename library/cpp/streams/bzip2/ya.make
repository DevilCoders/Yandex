LIBRARY()

OWNER(
    pg
    g:util
)

PEERDIR(
    contrib/libs/libbz2
)

SRCS(
    bzip2.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
