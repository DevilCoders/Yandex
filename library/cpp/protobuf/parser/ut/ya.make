UNITTEST_FOR(library/cpp/protobuf/parser)

OWNER(galaxycrab)

# parser_ut.cpp spends a lot of build time in release configuration (>20 min.)
NO_OPTIMIZE()

SRCS(
    parser_ut.proto
    parser_ut.cpp
    parser_ut_1.cpp
    parser_ut_2.cpp
    parser_ut_3.cpp
    parser_ut_4.cpp
    parser_ut_5.cpp
    parser_ut_6.cpp
    parser_ut_7.cpp
    base_types_ut.cpp
)

END()
