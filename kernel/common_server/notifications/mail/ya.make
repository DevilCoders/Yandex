LIBRARY()

OWNER(g:cs_dev)

SRCS(
    attachment.cpp
    binder.cpp
    GLOBAL mail.cpp
    request.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    kernel/common_server/notifications/abstract
    kernel/common_server/library/async_proxy
    kernel/common_server/library/json
    kernel/common_server/library/metasearch/simple
    kernel/common_server/util/network
)

GENERATE_ENUM_SERIALIZATION(binder.h)

END()
