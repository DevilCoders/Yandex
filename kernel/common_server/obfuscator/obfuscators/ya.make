LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/interfaces
    kernel/common_server/library/storage
    kernel/common_server/abstract
    kernel/common_server/util/json_scanner
)

SRCS(
    abstract.cpp
    GLOBAL fake.cpp
    GLOBAL total.cpp
    GLOBAL white_list.cpp
    GLOBAL with_policy.cpp
    GLOBAL data_transformation.cpp
    GLOBAL obfuscate_operator.cpp
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(abstract.h)
GENERATE_ENUM_SERIALIZATION_WITH_HEADER(obfuscate_operator.h)

END()
