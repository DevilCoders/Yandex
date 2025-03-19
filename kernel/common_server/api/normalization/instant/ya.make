LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/util
)

SRCS(
    GLOBAL normalizer.cpp
)

GENERATE_ENUM_SERIALIZATION(
    normalizer.h
)

END()
