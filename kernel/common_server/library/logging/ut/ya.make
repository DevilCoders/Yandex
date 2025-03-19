UNITTEST_FOR(kernel/common_server/library/logging)

OWNER(g:cs_dev)

SIZE(SMALL)

PEERDIR(
    library/cpp/testing/unittest
    kernel/common_server/library/unistat
    library/cpp/logger
)

SRCS(
    events_ut.cpp
)

END()
