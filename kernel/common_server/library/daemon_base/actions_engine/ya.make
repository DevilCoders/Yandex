LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/digest/md5
    library/cpp/http/io
    library/cpp/json
    library/cpp/json/writer
    library/cpp/logger/global
    library/cpp/object_factory
    kernel/common_server/library/cluster
    kernel/common_server/library/sharding
    kernel/common_server/library/tasks_graph
    kernel/common_server/util
)

SRCS(
    controller_client.cpp
    GLOBAL controller_script.cpp
)

END()
