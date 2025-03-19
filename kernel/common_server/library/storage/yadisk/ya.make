LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/simple_client
    kernel/common_server/library/disk
)

SRCS(
    GLOBAL storage.cpp
    GLOBAL config.cpp
)

END()
