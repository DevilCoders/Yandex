LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/util
    kernel/common_server/api/snapshots/storage
    kernel/common_server/api/snapshots/fetching
    kernel/common_server/api/snapshots/removing
    kernel/common_server/api/snapshots/cleaning
    kernel/common_server/api/snapshots/objects

)

GENERATE_ENUM_SERIALIZATION(
    object.h
)

GENERATE_ENUM_SERIALIZATION(
    group.h
)

SRCS(
    config.cpp
    manager.cpp
    object.cpp
    group.cpp
    abstract.cpp
    controller.cpp
)

END()
