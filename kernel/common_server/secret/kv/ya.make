LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/util
    kernel/common_server/library/kv
)

SRCS(
    GLOBAL config.cpp
    manager.cpp
)

END()
