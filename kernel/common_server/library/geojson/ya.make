LIBRARY()

GENERATE_ENUM_SERIALIZATION(value.h)

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/geometry
    kernel/common_server/library/json
)

SRCS(
    value.cpp
)

END()
