LIBRARY()

OWNER(g:yabs-rt)

PEERDIR(
    library/cpp/monlib/dynamic_counters
    library/cpp/safe_stats
)

SRCS(
    dynamic_counters.cpp
)

END()
