UNITTEST()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/http/server
    library/cpp/neh
    kernel/common_server/library/async_proxy
    kernel/common_server/library/async_proxy/ut/helper
    kernel/common_server/util/logging
    kernel/common_server/util/network
    search/meta/scatter/options
    search/meta/scatter
    kernel/httpsearchclient
)

SRCDIR(kernel/common_server/library/async_proxy)

SRCS(
    async_proxy_ut.cpp
)

END()
