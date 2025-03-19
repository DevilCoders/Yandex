LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/proto
    kernel/common_server/library/storage
    kernel/common_server/library/unistat
    kernel/common_server/library/interfaces
    kernel/common_server/notifications/abstract
    kernel/common_server/util
    kernel/common_server/abstract
    library/cpp/deprecated/atomic
)

SRCS(
    db_entities.cpp
    db_entities_history.cpp
    db_owner.cpp
    db_direct.cpp
    config.cpp
    filter.cpp
    sequential.cpp
    event.cpp
    manager.cpp
    session.cpp
    cache.cpp
    common.cpp
    db_preparation.cpp
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(filter.h)

END()
