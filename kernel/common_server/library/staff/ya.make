LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/json
    kernel/common_server/util
    kernel/common_server/library/tvm_services/abstract
    kernel/common_server/library/tvm_services/abstract/request
)

SRCS(
    client.cpp
    entry.cpp
)

GENERATE_ENUM_SERIALIZATION(entry.h)

END()

RECURSE_FOR_TESTS(
    ut_large
)
