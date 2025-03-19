LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/api/links
    kernel/common_server/library/storage
    kernel/common_server/roles/abstract
    kernel/common_server/user_role/abstract
    kernel/common_server/util
)

SRCS(
    config.cpp
    manager.cpp
)

END()
