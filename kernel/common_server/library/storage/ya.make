LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/charset
    library/cpp/digest/md5
    library/cpp/logger/global
    library/cpp/mediator/global_notifications
    library/cpp/regex/pcre
    library/cpp/string_utils/base64
    library/cpp/yconf
    kernel/common_server/util/logging
    kernel/common_server/library/logging
    kernel/common_server/library/storage/abstract
    kernel/common_server/library/storage/reply
    kernel/common_server/library/storage/records
    kernel/common_server/library/storage/query
    kernel/common_server/library/storage/proto
    kernel/common_server/library/storage/balancing
    kernel/common_server/library/storage/selection
    kernel/common_server/util
    search/idl
)

SRCS(
    abstract.cpp
    config.cpp
    structured.cpp
    cache_storage.cpp
)

END()

RECURSE_FOR_TESTS (
    postgres/ut
    selection/ut
    ut
    ydb/ut
)
