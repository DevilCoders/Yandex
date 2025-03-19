UNITTEST_FOR(kernel/common_server/library/logging/record)

OWNER(g:cs_dev)

SIZE(SMALL)

PEERDIR(
    library/cpp/testing/unittest
    library/cpp/logger
    kernel/common_server/library/logging
)

SRCS(
    record_ut.cpp
)

END()
