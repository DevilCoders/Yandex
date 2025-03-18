OWNER(
    g:geoapps_infra
)

RECURSE(generated_files_updater)

BOOSTTEST_WITH_MAIN()


PEERDIR(
    ADDINCL tools/idl/generator
    library/cpp/testing/common
)

SRCS(
    doc_maker_tests.cpp
    internal_tag_tests.cpp
    objc/low_level_tests.cpp
    protoconv/protoconv_tests.cpp
    test_helpers.cpp
)

TEST_CWD(tools/idl)

DATA(arcadia/tools/idl/generator/tests/generated_files)

END()
