LIBRARY()

OWNER(g:cs_dev)

SRCS(
    helper.cpp
    fixed_response_server.cpp
)

PEERDIR(
    kernel/httpsearchclient
    library/cpp/http/server
    library/cpp/neh
    kernel/common_server/library/async_proxy
    kernel/common_server/library/neh/server
    kernel/common_server/util/logging
    search/meta/scatter
    search/meta/scatter/options
)

END()
