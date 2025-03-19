LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/query
    kernel/common_server/library/interfaces
)

SRCS(
    config.cpp
    database.cpp
    query_result.cpp
    transaction.cpp
    table.cpp
    lock.cpp
    statement.cpp
)

GENERATE_ENUM_SERIALIZATION(statement.h)
GENERATE_ENUM_SERIALIZATION(database.h)
GENERATE_ENUM_SERIALIZATION(transaction.h)

END()
