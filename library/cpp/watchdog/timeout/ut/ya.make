UNITTEST_FOR(library/cpp/watchdog/timeout)

OWNER(vmordovin)

PEERDIR(
    library/cpp/threading/future
    library/cpp/watchdog
)

SRCS(
    watchdog_ut.cpp
)

END()
