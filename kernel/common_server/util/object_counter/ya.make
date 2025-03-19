LIBRARY()

OWNER(g:cs_dev)

SRCS(
    object_counter.cpp
    object_counter_helper.cpp
    stats_sender.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
