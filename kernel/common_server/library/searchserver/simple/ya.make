LIBRARY()

OWNER(g:cs_dev)

SRCS(
    protocol.cpp
    client.cpp
    exception.cpp
    server.cpp
    replier.cpp
    delay.cpp
    http_status_config.cpp
)

PEERDIR(
    kernel/common_server/library/logging
    kernel/common_server/library/openssl
    kernel/common_server/library/report_builder
    kernel/common_server/library/unistat
    kernel/common_server/util
    kernel/daemon
    kernel/reqerror
    kernel/reqid
    kernel/search_daemon_iface
    library/cpp/cgiparam
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/http/static
    library/cpp/logger/global
    library/cpp/streams/lz
    library/cpp/yconf
    search/common
    search/output_context
)

END()
