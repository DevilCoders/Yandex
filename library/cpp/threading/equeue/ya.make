LIBRARY()

OWNER(
    ironpeter
    mvel
    g:base
    g:middle
)

SRCS(
    equeue.h
    equeue.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
