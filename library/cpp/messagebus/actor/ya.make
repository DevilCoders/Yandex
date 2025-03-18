LIBRARY(messagebus_actor)

OWNER(g:messagebus)

SRCS(
    executor.cpp
    thread_extra.cpp
    what_thread_does.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
