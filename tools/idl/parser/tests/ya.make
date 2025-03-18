OWNER(
    g:geoapps_infra
)

BOOSTTEST_WITH_MAIN()

PEERDIR(
    tools/idl/parser
)

SRCS(
    test_helpers.cpp
    custom_code_link_tests.cpp
    doc_tests.cpp
    enum_tests.cpp
    error_tests.cpp
    framework_parsing_test.cpp
    general_tests.cpp
    interface_tests.cpp
    listener_tests.cpp
    name_tests.cpp
    struct_tests.cpp
    type_tests.cpp
    variant_tests.cpp
)

TEST_CWD(tools/idl)

DATA(arcadia/tools/idl/parser/tests)

END()
