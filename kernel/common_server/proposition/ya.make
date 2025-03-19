LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/api/links
    kernel/common_server/library/storage
    kernel/common_server/util
    kernel/common_server/proposition/actions
    kernel/common_server/proposition/policies
)

SRCS(
    config.cpp
    manager.cpp
    object.cpp
    verdict.cpp
    common.cpp
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(common.h)

END()
