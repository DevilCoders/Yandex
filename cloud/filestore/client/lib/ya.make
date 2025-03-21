LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    add_cluster_node.cpp
    command.cpp
    create.cpp
    describe.cpp
    destroy.cpp
    factory.cpp
    kick_endpoint.cpp
    list_cluster_nodes.cpp
    list_endpoints.cpp
    list_filestores.cpp
    ls.cpp
    mkdir.cpp
    mount.cpp
    read.cpp
    remove_cluster_node.cpp
    resize.cpp
    rm.cpp
    start_endpoint.cpp
    stop_endpoint.cpp
    touch.cpp
    write.cpp
)

PEERDIR(
    cloud/filestore/libs/client
    cloud/filestore/libs/diagnostics
    cloud/filestore/libs/fuse

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics

    library/cpp/actors/util
    library/cpp/getopt
    library/cpp/logger
    library/cpp/protobuf/json
    library/cpp/threading/future
)

END()
