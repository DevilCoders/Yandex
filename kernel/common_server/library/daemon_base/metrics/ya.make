LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/logger/global
    library/cpp/mediator
    yweb/fleur/util/metrics
)

SRCS(
    metrics.cpp
    persistent.cpp
    servicemetrics.cpp
)

END()
