LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/query
)

SRCS(
    abstract.cpp
    filter.cpp
    sorting.cpp
    selection.cpp
    iterator.cpp
    multi_iterator.cpp
    cursor.cpp
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(filter.h)

END()
