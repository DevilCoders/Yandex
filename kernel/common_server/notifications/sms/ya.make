LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL sms.cpp
    request.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/tvmauth/client
    library/cpp/xml/sax
    kernel/common_server/notifications/abstract
    kernel/common_server/library/async_proxy
    kernel/common_server/library/json
    kernel/common_server/library/metasearch/simple
    kernel/common_server/library/unistat
    kernel/common_server/util/network
)

END()
