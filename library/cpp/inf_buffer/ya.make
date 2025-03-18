LIBRARY()

OWNER(kostik)

SRCS(
    inf_buffer.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
