LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/json
    library/cpp/json/writer
    kernel/common_server/library/async_impl
    kernel/common_server/util
    library/cpp/string_utils/url
)

SRCS(
    client.cpp
    config.cpp
    entity.cpp
)

GENERATE_ENUM_SERIALIZATION(logger.h)
GENERATE_ENUM_SERIALIZATION(entity.h)

END()
