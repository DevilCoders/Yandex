LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/filestore/gateway/nfs/libs/api

    cloud/filestore/config
    cloud/filestore/libs/client
    cloud/filestore/libs/service

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics

    library/cpp/protobuf/util
)

SRCS(
    config.cpp
    convert.cpp
    factory.cpp
    service.cpp
    service_attr.cpp
    service_cluster.cpp
    service_list.cpp
    service_node.cpp
    service_stat.cpp
)

END()

RECURSE_FOR_TESTS(ut)
