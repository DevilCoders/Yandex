LIBRARY()

OWNER(g:cs_dev)

SRCS(
    addr.cpp
    async_delivery.cpp
    broadcast_collector.cpp
    message.cpp
    rate.cpp
    report.cpp
    shard.cpp
    shard_source.cpp
    shards_report.cpp
)

PEERDIR(
    kernel/common_server/library/metasearch/simple
    kernel/common_server/library/searchserver/simple
    kernel/common_server/library/signals
    kernel/common_server/library/unistat
    kernel/common_server/util
    kernel/httpsearchclient
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/json/writer
    library/cpp/logger/global
    library/cpp/messagebus/scheduler
    library/cpp/neh
    library/cpp/string_utils/base64
    library/cpp/unistat
    library/cpp/yconf
    search/meta/scatter
    search/meta/scatter/options
    search/session/logger
    kernel/common_server/util
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
