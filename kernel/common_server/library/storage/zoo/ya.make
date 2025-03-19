LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/cache
    library/cpp/zookeeper
    kernel/common_server/library/storage
)

SRCS(
    GLOBAL zoo_storage.cpp
    GLOBAL config.cpp
    lock.cpp
    owner.cpp
)

END()
