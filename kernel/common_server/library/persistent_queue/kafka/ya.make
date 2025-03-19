LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL config.cpp
    message.cpp
    queue.cpp
)

ADDINCL(
    contrib/libs/librdkafka/include
)

PEERDIR(
    contrib/libs/cppkafka
    library/cpp/digest/md5
    library/cpp/logger/global
    library/cpp/mediator/global_notifications
    kernel/common_server/library/logging
    kernel/common_server/library/persistent_queue/abstract
    kernel/common_server/util
    kernel/common_server/util/logging
)

GENERATE_ENUM_SERIALIZATION(config.h)

END()

RECURSE_FOR_TESTS(
    ut
)
