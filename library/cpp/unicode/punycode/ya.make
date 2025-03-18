LIBRARY()

OWNER(
    g:base
    g:middle
    g:upper
)

PEERDIR(
    contrib/libs/libidn
)

SRCS(
    punycode.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
