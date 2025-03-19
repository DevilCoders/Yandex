LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/logger/global
    library/cpp/monlib/dynamic_counters
    library/cpp/monlib/metrics
    library/cpp/monlib/service
    library/cpp/monlib/dynamic_counters
    library/cpp/monlib/service/pages
    library/cpp/http/client
    library/cpp/messagebus/scheduler
    library/cpp/mediator/global_notifications
    library/cpp/yconf
)

SRCS(
    signals.cpp
    cache.cpp
)

END()
