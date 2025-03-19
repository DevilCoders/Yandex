LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/interfaces
    kernel/common_server/library/logging
    kernel/common_server/util
    kernel/common_server/library/kv/abstract
)

SRCS(
    storage.cpp
    GLOBAL config.cpp
)

END()
