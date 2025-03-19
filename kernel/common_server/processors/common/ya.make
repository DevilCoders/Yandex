LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/auth
    kernel/common_server/user_auth/abstract
    kernel/common_server/user_role
    kernel/common_server/user_role/abstract
    kernel/common_server/processors/common/forward
)

SRCS(
    handler.cpp
    GLOBAL config.cpp
)

END()
