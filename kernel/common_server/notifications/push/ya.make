LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL push.cpp
)

PEERDIR(
    kernel/reqid
    library/cpp/yconf
    kernel/common_server/util/network
    kernel/common_server/notifications/abstract
    kernel/common_server/library/async_proxy
    kernel/common_server/util/network
    library/cpp/string_utils/quote
)

END()
