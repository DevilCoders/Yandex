LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/interfaces
    kernel/common_server/library/logging
    kernel/common_server/util
    kernel/common_server/abstract
    kernel/common_server/library/kv/abstract
    kernel/common_server/library/tvm_services
    contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3
)

SRCS(
    storage.cpp
    GLOBAL config.cpp
)

END()
