PY2TEST()

OWNER(arcadia-devtools)

TEST_SRCS(
    test1.py
    test2.py
)

DATA(
    arcadia/devtools/dummy_arcadia/pytests-samples
)

PEERDIR(
    library/python/testing/pytest_runner
)

REQUIREMENTS(container:387074777)

SIZE(LARGE)

TAG(
    ya:fat
    ya:force_sandbox
)

NO_CHECK_IMPORTS()

# this is mandatory to be able to have multiple files with pytest runs
FORK_TEST_FILES()

END()
