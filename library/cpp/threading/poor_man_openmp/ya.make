LIBRARY()

OWNER(agorodilov)

SRCS(
    thread_helper.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
