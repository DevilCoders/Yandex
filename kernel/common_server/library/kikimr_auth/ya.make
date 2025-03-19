LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/interfaces
    kernel/common_server/library/logging
    kernel/common_server/util
    kikimr/public/sdk/cpp/client/tvm
    library/cpp/digest/md5
    library/cpp/mediator/global_notifications
    library/cpp/tvmauth/client
    library/cpp/yconf
    ydb/public/sdk/cpp/client/ydb_driver
)

SRCS(
    config.cpp
)

GENERATE_ENUM_SERIALIZATION(config.h)

END()
