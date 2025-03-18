UNITTEST_FOR(library/cpp/auth_client_parser)

OWNER(g:passport_infra)

SRCS(
    oauth_token_ut.cpp
    parse_method_ut.cpp
    parse_method_v2_ut.cpp
    parse_method_v3_ut.cpp
    parse_multi_ut.cpp
    parse_multi_v2_ut.cpp
    parse_multi_v3_ut.cpp
    parse_utils_ut.cpp
    utils.cpp
)

END()
