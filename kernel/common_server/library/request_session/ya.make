LIBRARY()

OWNER(
    g:cs_dev
    dkozhevn
)

PEERDIR(
    kernel/common_server/library/request_session/proto
    kernel/common_server/library/logging
    kernel/common_server/library/scheme
    library/cpp/json
)

SRCS(
    request_session.cpp
)

END()

RECURSE_FOR_TESTS(ut)
