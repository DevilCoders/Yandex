LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/abstract
    kernel/common_server/api/history
    kernel/common_server/proto
    kernel/common_server/library/storage
    kernel/common_server/rt_background/processes/dumper
    kernel/common_server/library/vname_checker
)

SRCS(
    object.cpp
    manager.cpp
    config.cpp
    abstract.cpp
    abstract_tag.cpp
    proto_tag.cpp
    special_tag.cpp
    objects_filter.cpp
    GLOBAL filter.cpp
    GLOBAL yt_dumper.cpp
)

GENERATE_ENUM_SERIALIZATION(abstract.h)
GENERATE_ENUM_SERIALIZATION(filter.h)

END()
