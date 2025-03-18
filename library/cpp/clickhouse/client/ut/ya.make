UNITTEST()

OWNER(stanly)

SRCS(
    columns_ut.cpp
    type_parser_ut.cpp
    types_ut.cpp
)

PEERDIR(
    library/cpp/clickhouse/client
)

END()
