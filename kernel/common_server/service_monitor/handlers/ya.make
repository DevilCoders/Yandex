LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL service_info.cpp
    GLOBAL controller_command.cpp
)

PEERDIR(
    library/cpp/protobuf/json
    kernel/common_server/processors/common
    kernel/common_server/service_monitor/handlers/proto
    kernel/common_server/util/network
    kernel/common_server/util/algorithm
    yp/cpp/yp
    yp/yp_proto/yp/client/api/proto
    library/cpp/protobuf/json
)

END()
