LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    kernel/common_server/api/snapshots
)

SRCS(
    GLOBAL processor.cpp
    GLOBAL snapshot.cpp
    GLOBAL snapshot_object.cpp
    GLOBAL snapshot_group.cpp
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(snapshot.h)

END()
