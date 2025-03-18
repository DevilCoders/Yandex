UNITTEST()

OWNER(g:middle)

PEERDIR(
    library/cpp/eventlog
    library/cpp/eventlog/proto
    library/cpp/threading/future
    library/cpp/deprecated/atomic
)

SRCS(
    eventlog_ut.cpp
    reopen_eventlog_ut.cpp
    threaded_eventlog_degradation_ut.cpp
    test_events.ev
)

DATA(sbr://2865030496)

IF (SANITIZER_TYPE)
    SIZE(MEDIUM)
ENDIF()

END()
