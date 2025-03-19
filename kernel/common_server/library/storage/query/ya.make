LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/proto
    kernel/common_server/library/interfaces
)

SRCS(
    abstract.cpp
    create_table.cpp
    remove_rows.cpp
    request.cpp
)

GENERATE_ENUM_SERIALIZATION(create_table.h)
GENERATE_ENUM_SERIALIZATION(request.h)

END()
