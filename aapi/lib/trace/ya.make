LIBRARY()

OWNER(g:aapi)

PEERDIR(
    library/cpp/eventlog
    library/cpp/sighandler
)

SRCS(
    events.ev
    trace.cpp
)

END()
