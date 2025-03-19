LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/kikimr_auth
    kernel/common_server/library/logging
    kernel/common_server/library/persistent_queue/abstract
    kernel/common_server/util
    kernel/common_server/util/algorithm
    kernel/common_server/util/logging
    kikimr/public/sdk/cpp/client/tvm
    kikimr/public/sdk/cpp/client/ydb_persqueue
    library/cpp/threading/future
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_persqueue_core
)

SRCS(
    GLOBAL config.cpp
    message.cpp
    queue.cpp
    reader.cpp
    writer.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
