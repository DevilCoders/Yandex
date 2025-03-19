LIBRARY()

OWNER(g:cs_dev)

SRCS(
    address.cpp
    http_request.cpp
    neh_request.cpp
    neh.cpp
    simple.cpp
)

GENERATE_ENUM_SERIALIZATION(http_request_enum.h)

PEERDIR(
    kernel/common_server/library/async_proxy
    kernel/common_server/library/metasearch/simple
    kernel/common_server/util
    library/cpp/digest/md5
    library/cpp/dns
    library/cpp/http/io
    library/cpp/json
    library/cpp/json/writer
    library/cpp/logger/global
    library/cpp/mediator/global_notifications
    library/cpp/neh
    library/cpp/openssl/io
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/threading/future
    library/cpp/yconf
    search/meta/scatter
    search/session/logger
    tools/enum_parser/enum_serialization_runtime
)

END()
