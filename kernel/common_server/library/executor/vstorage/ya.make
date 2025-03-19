LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/object_factory
    library/cpp/yconf
    kernel/common_server/library/executor
    kernel/common_server/library/executor/abstract
    kernel/common_server/library/executor/proto
    kernel/common_server/library/unistat
)

SRCS(
    GLOBAL storage.cpp
    GLOBAL queue.cpp
)

END()
