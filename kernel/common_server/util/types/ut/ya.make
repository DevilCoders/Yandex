UNITTEST_FOR(kernel/common_server/util/types)

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/logger/global
    kernel/common_server/util/types
)

SRCS(
    coverage_ut.cpp
    expected_ut.cpp
    string_pool_ut.cpp
)

END()
