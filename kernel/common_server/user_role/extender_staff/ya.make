LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/staff
    kernel/common_server/user_role/abstract
)

SRCS(
    GLOBAL extender_staff.cpp
)

END()
