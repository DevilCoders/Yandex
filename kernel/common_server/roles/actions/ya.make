LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/abstract
    kernel/common_server/roles/abstract
    kernel/common_server/user_role/abstract
    kernel/common_server/util
    tools/enum_parser/enum_serialization_runtime
)

GENERATE_ENUM_SERIALIZATION(common.h)

SRCS(
    common.cpp
)

END()
