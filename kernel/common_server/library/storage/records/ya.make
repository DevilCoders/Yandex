LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/query
    kernel/common_server/library/storage/proto
)

SRCS(
    abstract.cpp
    record.cpp
    set.cpp
    db_value.cpp
    db_value_input.cpp
    t_record.cpp
)

GENERATE_ENUM_SERIALIZATION(abstract.h)
GENERATE_ENUM_SERIALIZATION(db_value.h)

END()
