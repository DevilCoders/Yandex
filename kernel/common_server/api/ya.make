LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/histogram/rt
    library/cpp/json
    library/cpp/yconf
    kernel/common_server/abstract
    kernel/common_server/api/history
    kernel/common_server/api/security
    kernel/common_server/api/links
    kernel/common_server/api/snapshots
    kernel/common_server/api/normalization
    kernel/common_server/api/localization
    kernel/common_server/library/logging
    kernel/common_server/library/geometry
    kernel/common_server/util/algorithm
    kernel/common_server/util/network
    kernel/common_server/library/storage
    search/fetcher
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(common.h)

SRCS(
    common.cpp
)

END()
