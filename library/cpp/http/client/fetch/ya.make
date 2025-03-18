LIBRARY()

OWNER(stanly)

SRCS(
    coctx.cpp
    fetch_request.cpp
    fetch_result.cpp
    fetch_single.cpp
    parse.cpp
    pool.cpp
)

PEERDIR(
    kernel/langregion
    library/cpp/charset
    library/cpp/coroutine/dns
    library/cpp/coroutine/engine
    library/cpp/http/client/ssl
    library/cpp/http/fetch_gpl
    library/cpp/http/io
    library/cpp/langs
    library/cpp/mime/types
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
    library/cpp/uri
    library/cpp/deprecated/atomic
)

END()
