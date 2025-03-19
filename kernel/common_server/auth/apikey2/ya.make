LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/auth/common
)

GENERATE_ENUM_SERIALIZATION(apikey2.h)

SRCS(
    GLOBAL apikey2.cpp
)

END()
