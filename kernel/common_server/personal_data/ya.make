LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/abstract
    kernel/common_server/proto
    kernel/common_server/util
    library/cpp/object_factory
    library/cpp/yconf
)

GENERATE_ENUM_SERIALIZATION(storage.h)

SRCS(
    GLOBAL storage.cpp
)

END()
