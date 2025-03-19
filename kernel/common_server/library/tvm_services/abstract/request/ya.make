LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/tvm_services/abstract/proto
    kernel/common_server/library/request_session
    kernel/common_server/util
    kernel/common_server/util/network
    kernel/common_server/library/logging
    kernel/common_server/library/scheme
    library/cpp/xml/document
    library/cpp/json
)

SRCS(
    abstract.cpp
    json.cpp
    xml.cpp
    raw_json.cpp
    direct.cpp
    ct_parser.cpp
    zip.cpp
)

GENERATE_ENUM_SERIALIZATION(abstract.h)
GENERATE_ENUM_SERIALIZATION_WITH_HEADER(
    ct_parser.h
)
END()
