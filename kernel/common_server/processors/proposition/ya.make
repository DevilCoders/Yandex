LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/common
)

SRCS(
    GLOBAL handler.cpp
    GLOBAL permission.cpp
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(permission.h)

END()
