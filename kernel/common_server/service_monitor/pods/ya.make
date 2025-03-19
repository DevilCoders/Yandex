LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL deploy_locators.cpp
    errors.cpp
    GLOBAL nanny_locators.cpp
    pod_locator.cpp
    GLOBAL resources_storage.cpp
    GLOBAL server_info_storage.cpp
)

GENERATE_ENUM_SERIALIZATION(
    pod_locator.h
)

PEERDIR(
    kernel/common_server/service_monitor/handlers/proto
    kernel/common_server/util/network
    rtline/library/metasearch/simple
    library/cpp/protobuf/json
    yp/yp_proto/yp/client/api/proto
)

END()
