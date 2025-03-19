LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/util/network
    kernel/common_server/library/logging
    kernel/common_server/library/tvm_services/abstract/customizer
    kernel/common_server/library/tvm_services/abstract/proto
    kernel/common_server/library/tvm_services/abstract/request
    library/cpp/json
    library/cpp/xml/document
    library/cpp/threading/future
    library/cpp/tvmauth/client
)

GENERATE_ENUM_SERIALIZATION(abstract.h)

SRCS(
    GLOBAL abstract.cpp
    emulator.cpp
)

END()
