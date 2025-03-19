LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/common
    kernel/common_server/rt_background
    kernel/common_server/user_auth/abstract
)

SRCS(
    GLOBAL user_auth.cpp
)

END()
