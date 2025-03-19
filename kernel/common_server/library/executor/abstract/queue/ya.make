LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/executor/proto
    kernel/common_server/library/storage
    kernel/common_server/library/unistat
)

GENERATE_ENUM_SERIALIZATION(common.h)

SRCS(
    abstract.cpp
    common.cpp
    GLOBAL view.cpp
)

END()
