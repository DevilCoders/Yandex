OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    client.cpp
    query.cpp
    request.cpp
    scheduler.cpp
)

PEERDIR(
    library/cpp/coroutine/dns
    library/cpp/coroutine/engine
    library/cpp/coroutine/util
    library/cpp/http/client/cookies
    library/cpp/http/client/fetch
    library/cpp/http/client/ssl
    library/cpp/uri
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
