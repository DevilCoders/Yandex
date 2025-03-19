LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/threading/named_lock
    kernel/common_server/library/storage
)

SRCS(
    GLOBAL local_storage.cpp
    GLOBAL config.cpp
)

END()
