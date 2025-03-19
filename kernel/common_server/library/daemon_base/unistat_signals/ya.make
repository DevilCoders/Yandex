LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/unistat
    library/cpp/logger/global
    library/cpp/mediator
    search/common_signals
)

SRCS(
    signals.cpp
)

END()
