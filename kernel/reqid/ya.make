LIBRARY()

OWNER(
    mvel
    g:base
    g:middle
)

SRCS(reqid.cpp)

PEERDIR(
    library/cpp/charset
    contrib/libs/re2
)

END()

RECURSE_FOR_TESTS(
    ut
)
