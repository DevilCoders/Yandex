PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    # This target is mandatory, it does all the job
    library/python/testing/types_test/py3
    library/python/testing/yatest_common
    # These targets are needed to include all the checked files and their
    # dependencies to test binary
    cloud/mdb/dbm/internal
)

TEST_SRCS(conftest.py)

DATA(arcadia/cloud/mdb/dbm/app.yaml)

DATA(arcadia/cloud/mdb/dbm/internal)

# Since mypy is rather slow it could be a good idea to force test
# to have MEDIUM or even LARGE size, and increase timeout.
SIZE(MEDIUM)

TIMEOUT(600)

END()
