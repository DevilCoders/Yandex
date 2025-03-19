LIBRARY()

OWNER(g:cs_dev)

SRCS(
    auto_actualization.cpp
    blob_with_header.cpp
    cgi_processing.cpp
    coded_exception.cpp
    instant_model.cpp
    json_processing.cpp
    map_processing.cpp
    queue.cpp
    smartqueue.cpp
    datacenter.cpp
    time_of_day.cpp
    objects_pool.cpp
    threading.cpp
    xml_processing.cpp
    destroyer.cpp
)

PEERDIR(
    library/cpp/balloc/optional
    library/cpp/json/writer
    library/cpp/logger/global
    kernel/common_server/library/unistat
    kernel/common_server/library/logging
    kernel/common_server/util/proto
    kernel/common_server/util/datetime
    contrib/libs/protobuf
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
