LIBRARY()

OWNER(g:cs_dev)

SRCS(
    client.cpp
    replier.cpp
    neh.cpp
)

PEERDIR(
    kernel/search_daemon_iface
    kernel/common_server/library/neh/server
    kernel/common_server/library/searchserver/simple
    kernel/common_server/library/unistat
    search/config
    search/daemons/httpsearch
    search/request/data
    search/session
)

END()
