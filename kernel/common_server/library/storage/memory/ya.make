LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage
)

SRCS(
    GLOBAL mem_storage.cpp
    GLOBAL config.cpp
)

END()
