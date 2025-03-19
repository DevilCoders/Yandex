LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/messagebus/scheduler
    library/cpp/object_factory
    library/cpp/yconf
    kernel/common_server/library/executor/abstract
    kernel/common_server/library/executor/clean
    kernel/common_server/library/executor/proto
    kernel/common_server/library/unistat
    kernel/common_server/util
)

GENERATE_ENUM_SERIALIZATION(executor.h)

SRCS(
    executor.cpp
)

END()
