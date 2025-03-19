LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/util
)

GENERATE_ENUM_SERIALIZATION(
    security.h
)

SRCS(
    security.cpp
)

END()
