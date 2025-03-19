LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/logger/global
    kernel/common_server/library/scheme
)

SRCS(
    checker.cpp
)

END()

RECURSE_FOR_TESTS (
    ut
)


