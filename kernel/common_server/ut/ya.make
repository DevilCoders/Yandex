LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/testing/unittest
    library/cpp/mediator
    kernel/common_server/server
)

SRCS(
    config.cpp
    test_base.cpp
    const.cpp
    server.cpp
)

END()
