LIBRARY()

OWNER(
    avitella
    mvel
    kulikov
    g:iss
    g:balancer
)

PEERDIR(
    library/cpp/config
    library/cpp/protobuf/util
    library/cpp/proto_config/protos
    library/cpp/colorizer
    library/cpp/getopt/small
    library/cpp/protobuf/json
    library/cpp/resource
    contrib/libs/protobuf
)

SRCS(
    config.cpp
    load.cpp
    usage.cpp
    json_to_proto_config.cpp
)

GENERATE_ENUM_SERIALIZATION(load.h)

END()
