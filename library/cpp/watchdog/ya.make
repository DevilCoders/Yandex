LIBRARY()

OWNER(
    g:base
    mvel
)

PEERDIR(
    library/cpp/watchdog/emergency
    library/cpp/watchdog/lib
    library/cpp/watchdog/port_check
    library/cpp/watchdog/resources
    library/cpp/watchdog/timeout
)

SRCS(
    watchdog.cpp
)

END()
