PY2TEST(pytest_runner-example-ut)

OWNER(arcadia-devtools)

TEST_SRCS(
    test0.py
    test0_empty_classname.py
    test0_name_prefix.py
    test3.py
    test_import_junit.py
    test_import_junit2.py
)

DATA(
    arcadia/devtools/dummy_arcadia/pytests-samples
    arcadia/devtools/dummy_arcadia/junit-samples
)

PEERDIR(
    library/python/testing/pytest_runner
)
#NO_LINT()
#NO_CHECK_IMPORTS()
DEPENDS(
    devtools/dummy_arcadia/pytest
)
# this is mandatory to be able to have multiple files with pytest runs
FORK_TEST_FILES()

END()
