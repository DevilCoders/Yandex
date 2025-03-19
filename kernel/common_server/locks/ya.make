LIBRARY()

OWNER(g:cs_dev)

SRCS(
    abstract.cpp
    GLOBAL local.cpp
    GLOBAL db.cpp
    GLOBAL fake.cpp
)

PEERDIR(
    kernel/common_server/library/storage
    library/cpp/mediator/global_notifications
    kernel/common_server/abstract
    library/cpp/threading/named_lock
)

END()
