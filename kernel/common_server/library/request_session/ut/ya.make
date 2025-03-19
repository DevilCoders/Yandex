UNITTEST_FOR(kernel/common_server/library/logging)

OWNER(g:cs_dev)

SIZE(SMALL)

PEERDIR(
    library/cpp/testing/unittest
    kernel/common_server/library/request_session
    kernel/common_server/util/network
)

SRCS(
    request_session_ut.cpp
)

END()
