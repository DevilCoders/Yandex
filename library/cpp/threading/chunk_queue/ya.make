LIBRARY()

OWNER(g:rtmr)

SRCS(
    queue.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
