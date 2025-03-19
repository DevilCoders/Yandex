LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/daemon/config
    library/cpp/testing/unittest
    kernel/common_server/library/executor
)

SRCS(
    config.cpp
)

END()
