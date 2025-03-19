LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/abstract
    kernel/common_server/user_role/abstract
    kernel/common_server/roles/db_roles
)

SRCS(
    GLOBAL config.cpp
    manager.cpp
    object.cpp
)

END()
