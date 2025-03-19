LIBRARY()

OWNER(g:cs_dev)

SRCS(
    area.cpp
    manager.cpp
)

PEERDIR(
    kernel/common_server/library/scheme
    kernel/common_server/util
    kernel/common_server/library/interfaces
    kernel/common_server/proto
    kernel/common_server/library/geometry
    library/cpp/object_factory
)

END()
