LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/logger/global
    library/cpp/json/writer
    kernel/common_server/library/time_restriction/proto
    kernel/common_server/util
    kernel/common_server/common
    kernel/common_server/obfuscator/obfuscators
)

SRCS(
    one_restriction.cpp
    pool.cpp
    time_restriction.cpp
)

GENERATE_ENUM_SERIALIZATION(one_restriction.h)

END()
