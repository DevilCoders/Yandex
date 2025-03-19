LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/common
    kernel/common_server/user_auth/abstract
    kernel/common_server/user_role/abstract
    kernel/common_server/roles/abstract
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(handler.h)

SRCS(
    GLOBAL handler.cpp
)

END()
