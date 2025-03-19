LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL telegram.cpp
)

GENERATE_ENUM_SERIALIZATION(telegram.h)

PEERDIR(
    library/cpp/yconf
    kernel/common_server/util/network
    kernel/common_server/notifications/abstract
    kernel/common_server/library/async_proxy
    kernel/common_server/util/network
    library/cpp/string_utils/quote
    library/cpp/html/escape
)

END()
