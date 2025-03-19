LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/common
    kernel/common_server/rt_background
    kernel/common_server/roles/abstract
    kernel/common_server/roles/db_roles
)

SRCS(
    GLOBAL idm.cpp
    GLOBAL item.cpp
    GLOBAL permissions.cpp
    GLOBAL role.cpp
)

END()
