LIBRARY()

OWNER(g:zootopia)

SRCS(
    time_accuracy.cpp
    time_stats_counters.cpp
)

PEERDIR(
    library/cpp/monlib/dynamic_counters
)

END()

RECURSE_FOR_TESTS(
    ut
)
