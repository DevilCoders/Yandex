LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/util
    kernel/common_server/util/network
    kernel/common_server/library/unistat
    library/cpp/http/misc
    library/cpp/json
)

SRCS(
    async_impl.cpp
    client.cpp
    config.cpp
    logger.cpp
)

GENERATE_ENUM_SERIALIZATION(client.h)

END()
