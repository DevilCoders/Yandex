LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL config.cpp
    queue.cpp
    object.cpp
)

ADDINCL(
    contrib/libs/librdkafka/include
)

PEERDIR(
    library/cpp/logger/global
    library/cpp/mediator/global_notifications
    kernel/common_server/library/logging
    kernel/common_server/library/persistent_queue/abstract
    kernel/common_server/library/storage
    kernel/common_server/util
    kernel/common_server/util/logging
)

END()
