LIBRARY()

OWNER(g:kikimr)

SRCS(
    activeactors.cpp
    activeactors.h
    flow_controlled_queue.cpp
    flow_controlled_queue.h
    future_callback.h
    mon_histogram_helper.h
    selfping_actor.cpp
)

PEERDIR(
    library/cpp/actors/core
    library/cpp/monlib/dynamic_counters
)

END()

RECURSE_FOR_TESTS(
    ut
)

