LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/cgiparam
    library/cpp/messagebus/scheduler
    library/cpp/object_factory
    library/cpp/string_utils/quote
    library/cpp/yconf
    kernel/common_server/library/executor/abstract/queue
    kernel/common_server/library/executor/proto
    kernel/common_server/util
)

SRCS(
    storage.cpp
    data_storage.cpp
    queue.cpp
    task.cpp
    data.cpp
    context.cpp
)

END()
