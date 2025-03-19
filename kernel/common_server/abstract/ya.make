LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/geoareas
    kernel/common_server/geobase
    kernel/common_server/library/tvm_services/abstract
    kernel/common_server/library/persistent_queue/abstract
    library/cpp/monlib/dynamic_counters
    library/cpp/monlib/service
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(common.h)
GENERATE_ENUM_SERIALIZATION_WITH_HEADER(frontend.h)

SRCS(
    external_api.cpp
    users_contacts.cpp
    frontend.cpp
    sessions.cpp
    common.cpp
)

END()
