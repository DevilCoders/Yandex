LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/abstract
    kernel/common_server/library/staff
)

GENERATE_ENUM_SERIALIZATION(abstract.h)

SRCS(
    abstract.cpp
)

END()
