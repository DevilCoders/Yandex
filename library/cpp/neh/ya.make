OWNER(
    and42 # former neh maintainer
    kulikov # current neh maintainer
    petrk
    pg
    g:middle
)

LIBRARY()

PEERDIR(
    contrib/libs/openssl
    library/cpp/containers/intrusive_rb_tree
    library/cpp/coroutine/engine
    library/cpp/coroutine/listener
    library/cpp/dns
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/http/push_parser
    library/cpp/neh/asio
    library/cpp/netliba/v6
    library/cpp/openssl/init
    library/cpp/openssl/method
    library/cpp/threading/atomic
    library/cpp/threading/thread_local
    library/cpp/deprecated/atomic
)

SRCS(
    conn_cache.cpp
    factory.cpp
    https.cpp
    http_common.cpp
    http_headers.cpp
    http2.cpp
    inproc.cpp
    jobqueue.cpp
    location.cpp
    multi.cpp
    multiclient.cpp
    netliba.cpp
    netliba_udp_http.cpp
    neh.cpp
    pipequeue.cpp
    rpc.cpp
    rq.cpp
    smart_ptr.cpp
    stat.cpp
    tcp.cpp
    tcp2.cpp
    udp.cpp
    utils.cpp
)

GENERATE_ENUM_SERIALIZATION(http_common.h)

END()

RECURSE_FOR_TESTS(
    ut
)
