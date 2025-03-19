LIBRARY()

OWNER(g:cs_dev)

GENERATE_ENUM_SERIALIZATION(structured.h)

PEERDIR(
    kernel/common_server/library/storage
    kernel/common_server/library/unistat
)

SRCS(
    structured.cpp
)

END()
