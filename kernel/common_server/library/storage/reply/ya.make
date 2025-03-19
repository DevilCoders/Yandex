LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/proto
)

SRCS(
    abstract.cpp
    parsed.cpp
    decoder.cpp
)

GENERATE_ENUM_SERIALIZATION(abstract.h)

END()
