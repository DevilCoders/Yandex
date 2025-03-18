OWNER(
    g:geoapps_infra
)

BOOSTTEST_WITH_MAIN()

PEERDIR(
    tools/idl/common
    tools/idl/utils
)

SRCS(
    test_helpers.cpp
    dummy_parser_impl.cpp
    validator_tests.cpp
)

END()
