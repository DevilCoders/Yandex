OWNER(
    g:rtmr
)

SUBSCRIBER(
    swarmer
)

LIBRARY()

SRCS(
    async.cpp
    core/future.cpp
    core/fwd.cpp
    fwd.cpp
    wait/fwd.cpp
    wait/wait.cpp
    wait/wait_group.cpp
    wait/wait_policy.cpp
)

END()

RECURSE(
    mt_ut
    perf
    ut
)
