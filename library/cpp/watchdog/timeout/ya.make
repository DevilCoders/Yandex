LIBRARY()

OWNER(
    g:base
    mvel
)

PEERDIR(
    library/cpp/watchdog/timeout/config
    library/cpp/watchdog/lib
)

SRCS(
    watchdog.cpp
)

END()

RECURSE(
    config
)
